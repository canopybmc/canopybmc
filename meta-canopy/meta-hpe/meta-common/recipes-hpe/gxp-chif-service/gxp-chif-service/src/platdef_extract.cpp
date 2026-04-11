// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH

// Extract the APML Platform Definition blob from the host BIOS SPI flash.
// The blob is stored inside a UEFI firmware volume as an FFS file,
// identified by well-known GUIDs. This replaces HPE's standalone
// hpe-platdef-extract utility with an in-process implementation.

#include "platdef_extract.hpp"

#include <phosphor-logging/lg2.hpp>

#include <zlib.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace chif
{

// UEFI packed structures for firmware volume parsing
#pragma pack(push, 1)

struct EfiGuid
{
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t data4[8];
};

struct EfiFvHeader
{
    uint8_t zeroVector[16];
    EfiGuid fileSystemGuid;
    uint64_t fvLength;
    uint32_t signature; // '_FVH' = 0x4856465F
    uint32_t attributes;
    uint16_t headerLength;
    uint16_t checksum;
    uint16_t extHeaderOffset;
    uint8_t reserved;
    uint8_t revision;
    // BlockMap follows but we don't need it
};

struct EfiFvExtHeader
{
    EfiGuid fvName;
    uint32_t extHeaderSize;
};

struct EfiFfsFileHeader
{
    EfiGuid name;
    uint16_t integrityCheck;
    uint8_t type;
    uint8_t attributes;
    uint8_t size[3];
    uint8_t state;
};

struct EfiSectionHeader
{
    uint8_t size[3];
    uint8_t type;
};

// PlatDef bundle and record structures (from HPE openbmc-chif-svc platdef.h)

// From HPE upstream platdef.h
#define PLATDEF_BUNDLE_SIGNATURE "$PlatdefBundle1$"

struct PlatDefBundleHeader
{
    char signature[16]; // PLATDEF_BUNDLE_SIGNATURE
    uint32_t totalSize;
    uint16_t flags;
    uint8_t headerLength;
    uint8_t count;
    uint8_t reserved[8];
};

struct PlatDefRecordHeader
{
    uint8_t type;
    uint8_t size; // record size in 16-byte units
    uint16_t recordId;
    uint16_t flags;
    uint8_t entityId;
    uint8_t entityInstance;
    uint8_t featureIndex;
    uint8_t reserved[3];
    uint32_t runtimeData;
    char name[16];
};

struct PlatDefTableData
{
    PlatDefRecordHeader header;
    char description[32];
    uint16_t flags;
    uint8_t majorVersion;
    uint8_t minorVersion;
    uint32_t buildTimestamp;
    uint32_t recordCount;
    uint32_t totalSize;
    uint8_t md5Hash[16];
    uint32_t compressedSize;
    uint8_t specialVersion;
    uint8_t buildVersion;
    uint8_t reserved[10];
};

static constexpr uint16_t tableDataFlagZLib = 0x0010;

// I2C mux control — CPLD variant (muxType == 1)
struct PlatDefI2CMuxCpld
{
    uint8_t muxType;
    uint8_t byte;        // CPLD register offset
    uint8_t initMask;
    uint8_t selectMask;
};

// I2C mux control union (16 bytes, only CPLD variant used here)
union PlatDefI2CMux
{
    uint8_t rawBytes[16];
    uint8_t muxType;
    PlatDefI2CMuxCpld cpld;
};

struct PlatDefI2CSegment
{
    uint32_t flags;
    uint8_t speed;
    uint8_t id;          // BIOS segment byte
    uint8_t parentId;
    uint8_t gpuRiserNumber;
    uint8_t reserved[8];
    PlatDefI2CMux muxControl;
};

// PlatDefPrimitive is a 16-byte union in HPE's platdef.h
struct PlatDefPrimitive
{
    uint8_t rawBytes[16];
};

// I2CEngine fixed fields between the record header and the segments array
struct PlatDefI2CEngineFixed
{
    PlatDefPrimitive reset;
    PlatDefPrimitive alert;
    uint32_t flags;
    uint8_t speed;
    uint8_t id;
    uint8_t count;
    uint8_t reserved[9];
};

#pragma pack(pop)

static_assert(sizeof(PlatDefBundleHeader) == 32,
              "PlatDefBundleHeader must be 32 bytes");
static_assert(sizeof(PlatDefRecordHeader) == 32,
              "PlatDefRecordHeader must be 32 bytes (Gen11 layout)");
static_assert(sizeof(PlatDefTableData) == 112,
              "PlatDefTableData must be 112 bytes");
static_assert(sizeof(PlatDefI2CSegment) == 32,
              "PlatDefI2CSegment must be 32 bytes");
static_assert(sizeof(PlatDefI2CMux) == 16,
              "PlatDefI2CMux must be 16 bytes");
static_assert(sizeof(PlatDefI2CEngineFixed) == 48,
              "PlatDefI2CEngineFixed must be 48 bytes");

static constexpr uint32_t efiFvhSignature = 0x4856465F; // '_FVH'
static constexpr uint8_t efiFvFileTypeFreeform = 0x02;
static constexpr uint8_t efiFfsLargeFile = 0x01;
static constexpr uint8_t efiSectionRaw = 0x19;
static constexpr uint8_t efiSectionGuidDefined = 0x02;

// APML firmware volume GUID: {7EBF5AB8-525E-417C-9B6B-5EF367856954}
static constexpr EfiGuid apmlFvGuid = {
    0x7EBF5AB8, 0x525E, 0x417C,
    {0x9B, 0x6B, 0x5E, 0xF3, 0x67, 0x85, 0x69, 0x54}};

// APML file GUID: {C5F6001C-39B4-43DD-9B9B-6832F1BB4BE9}
static constexpr EfiGuid apmlFileGuid = {
    0xC5F6001C, 0x39B4, 0x43DD,
    {0x9B, 0x9B, 0x68, 0x32, 0xF1, 0xBB, 0x4B, 0xE9}};

// EFI FFS2 file system GUID: {8C8CE578-8A3D-4F1C-9935-896185C32DD3}
static constexpr EfiGuid ffs2Guid = {
    0x8C8CE578, 0x8A3D, 0x4F1C,
    {0x99, 0x35, 0x89, 0x61, 0x85, 0xC3, 0x2D, 0xD3}};

static bool guidEqual(const EfiGuid& a, const EfiGuid& b)
{
    return std::memcmp(&a, &b, sizeof(EfiGuid)) == 0;
}

static bool guidIsAllFf(const EfiGuid& g)
{
    static constexpr EfiGuid allFf = {
        0xFFFFFFFF, 0xFFFF, 0xFFFF,
        {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
    return guidEqual(g, allFf);
}

static uint32_t ffsFileSize(const EfiFfsFileHeader& hdr)
{
    return static_cast<uint32_t>(hdr.size[0]) |
           (static_cast<uint32_t>(hdr.size[1]) << 8) |
           (static_cast<uint32_t>(hdr.size[2]) << 16);
}

// Find the MTD device path for a partition by label
static std::string findMtdByLabel(const std::string& label)
{
    const std::filesystem::path mtdBase = "/sys/class/mtd";
    if (!std::filesystem::exists(mtdBase))
    {
        return {};
    }

    for (const auto& entry : std::filesystem::directory_iterator(mtdBase))
    {
        // Skip read-only mtdNro entries
        std::string devName = entry.path().filename().string();
        if (devName.ends_with("ro"))
        {
            continue;
        }

        auto namePath = entry.path() / "name";
        if (!std::filesystem::exists(namePath))
        {
            continue;
        }
        std::ifstream nameFile(namePath);
        if (!nameFile)
        {
            continue;
        }
        std::string name;
        std::getline(nameFile, name);
        if (name == label)
        {
            // Return /dev/mtdN
            return "/dev/" + entry.path().filename().string();
        }
    }
    return {};
}

// Decompress the PlatDef blob from the raw APML file.
// Layout: [PlatDefBundleHeader][PlatDefTableData][zlib compressed records...]
// Returns: [PlatDefTableData header][decompressed records]
static std::vector<uint8_t> decompressPlatDef(const std::vector<uint8_t>& raw)
{
    // The APML file starts with a PlatDefBundleHeader followed by
    // a PlatDefTableData record. HPE skips the bundle header to reach
    // the table data.
    if (raw.size() < sizeof(PlatDefBundleHeader))
    {
        lg2::error("PlatDef: blob too small for bundle header");
        return {};
    }

    if (std::memcmp(raw.data(), PLATDEF_BUNDLE_SIGNATURE,
                    sizeof(PLATDEF_BUNDLE_SIGNATURE) - 1) != 0)
    {
        lg2::error("PlatDef: invalid bundle signature");
        return {};
    }

    size_t offset = sizeof(PlatDefBundleHeader);

    if (offset + sizeof(PlatDefTableData) > raw.size())
    {
        lg2::error("PlatDef: blob too small for table data header");
        return {};
    }

    PlatDefTableData tableHdr{};
    std::memcpy(&tableHdr, raw.data() + offset, sizeof(tableHdr));

    std::string_view desc(tableHdr.description,
                          sizeof(tableHdr.description));
    if (auto nul = desc.find('\0'); nul != std::string_view::npos)
    {
        desc = desc.substr(0, nul);
    }

    lg2::info("PlatDef: table \"{DESC}\" v{MAJ}.{MIN}, "
              "records={CNT}, compressedSize={CSZ}",
              "DESC", std::string(desc),
              "MAJ", tableHdr.majorVersion,
              "MIN", tableHdr.minorVersion,
              "CNT", tableHdr.recordCount,
              "CSZ", tableHdr.compressedSize);

    if (!(tableHdr.flags & tableDataFlagZLib))
    {
        // Not compressed — return data starting from the table header
        lg2::info("PlatDef: not compressed, returning raw records");
        return std::vector<uint8_t>(raw.begin() + offset, raw.end());
    }

    // Compressed: zlib data starts after the PlatDefTableData header
    size_t compOffset = offset + sizeof(PlatDefTableData);
    if (tableHdr.compressedSize < sizeof(PlatDefTableData))
    {
        lg2::error("PlatDef: invalid compressedSize {SZ}",
                   "SZ", tableHdr.compressedSize);
        return {};
    }
    uint32_t compSize = tableHdr.compressedSize - sizeof(PlatDefTableData);

    if (compOffset + compSize > raw.size())
    {
        lg2::error("PlatDef: compressed data exceeds blob size");
        return {};
    }

    // Decompress into output buffer: [PlatDefTableData][decompressed records]
    uLong decompSize = tableHdr.totalSize; // uLong: passed by pointer to zlib
    if (decompSize == 0 || decompSize > 512 * 1024)
    {
        decompSize = 256 * 1024; // fallback cap
    }
    std::vector<uint8_t> result(sizeof(PlatDefTableData) + decompSize);

    // Copy the table header as-is
    std::memcpy(result.data(), &tableHdr, sizeof(PlatDefTableData));

    int rc = ::uncompress(
        result.data() + sizeof(PlatDefTableData), &decompSize,
        raw.data() + compOffset, compSize);

    if (rc != Z_OK)
    {
        lg2::error("PlatDef: zlib decompress failed, rc={RC}", "RC", rc);
        return {};
    }

    result.resize(sizeof(PlatDefTableData) + decompSize);
    lg2::info("PlatDef: decompressed {SZ} bytes ({CNT} records)",
              "SZ", decompSize, "CNT", tableHdr.recordCount);
    return result;
}

std::vector<uint8_t> extractPlatDef()
{
    std::string mtdPath = findMtdByLabel("host-prime");
    if (mtdPath.empty())
    {
        lg2::info("PlatDef: host-prime MTD partition not found");
        return {};
    }

    lg2::info("PlatDef: reading from {PATH}", "PATH", mtdPath);

    std::ifstream rom(mtdPath, std::ios::binary | std::ios::ate);
    if (!rom)
    {
        lg2::error("PlatDef: failed to open {PATH}", "PATH", mtdPath);
        return {};
    }

    auto pos = rom.tellg();
    if (pos <= 0)
    {
        lg2::error("PlatDef: failed to determine ROM size");
        return {};
    }
    auto romSize = static_cast<size_t>(pos);
    rom.seekg(0);

    // Scan for firmware volumes every 64KB
    for (size_t fvOffset = 0; fvOffset + sizeof(EfiFvHeader) < romSize;
         fvOffset += 0x10000)
    {
        EfiFvHeader fvHdr{};
        rom.seekg(static_cast<std::streamoff>(fvOffset));
        rom.read(reinterpret_cast<char*>(&fvHdr), sizeof(fvHdr));
        if (!rom.good())
        {
            break;
        }

        if (fvHdr.signature != efiFvhSignature)
        {
            continue;
        }

        if (!guidEqual(fvHdr.fileSystemGuid, ffs2Guid))
        {
            continue;
        }

        if (fvHdr.extHeaderOffset == 0)
        {
            continue;
        }

        // Read extended header to check the volume name GUID
        EfiFvExtHeader extHdr{};
        rom.seekg(
            static_cast<std::streamoff>(fvOffset + fvHdr.extHeaderOffset));
        rom.read(reinterpret_cast<char*>(&extHdr), sizeof(extHdr));
        if (!rom.good())
        {
            continue;
        }

        if (!guidEqual(extHdr.fvName, apmlFvGuid))
        {
            continue;
        }

        lg2::info("PlatDef: found APML FV at offset 0x{OFF}",
                  "OFF", lg2::hex, fvOffset);

        // Walk FFS file entries starting after the extended header
        size_t fileOffset = fvOffset + fvHdr.extHeaderOffset +
                            extHdr.extHeaderSize;
        size_t fvEnd = fvOffset + static_cast<size_t>(fvHdr.fvLength);
        if (fvEnd > romSize)
        {
            fvEnd = romSize;
        }

        while (fileOffset + sizeof(EfiFfsFileHeader) < fvEnd)
        {
            // 8-byte align
            if (fileOffset & 0x07)
            {
                fileOffset = (fileOffset & ~0x07UL) + 0x08;
            }

            EfiFfsFileHeader fileHdr{};
            rom.seekg(static_cast<std::streamoff>(fileOffset));
            rom.read(reinterpret_cast<char*>(&fileHdr), sizeof(fileHdr));
            if (!rom.good())
            {
                break;
            }

            // End of file list
            if (guidIsAllFf(fileHdr.name))
            {
                break;
            }

            uint32_t fileSize = ffsFileSize(fileHdr);
            if (fileSize < sizeof(EfiFfsFileHeader))
            {
                break;
            }

            uint32_t dataSize = fileSize - sizeof(EfiFfsFileHeader);
            size_t dataOffset = fileOffset + sizeof(EfiFfsFileHeader);

            if (guidEqual(fileHdr.name, apmlFileGuid) &&
                !(fileHdr.attributes & efiFfsLargeFile))
            {
                lg2::info("PlatDef: found APML file, {SZ} bytes at 0x{OFF}",
                          "SZ", dataSize, "OFF", lg2::hex, dataOffset);

                // Handle section header for FREEFORM files
                if (fileHdr.type == efiFvFileTypeFreeform &&
                    dataSize > sizeof(EfiSectionHeader))
                {
                    EfiSectionHeader secHdr{};
                    rom.seekg(static_cast<std::streamoff>(dataOffset));
                    rom.read(reinterpret_cast<char*>(&secHdr),
                             sizeof(secHdr));
                    if (!rom.good())
                    {
                        lg2::error("PlatDef: section header read failed");
                        return {};
                    }

                    if (secHdr.type == efiSectionRaw)
                    {
                        dataOffset += sizeof(EfiSectionHeader);
                        dataSize -= sizeof(EfiSectionHeader);
                    }
                    else if (secHdr.type == efiSectionGuidDefined)
                    {
                        constexpr uint32_t guidSecHdrSize = 20;
                        if (dataSize <= guidSecHdrSize)
                        {
                            break;
                        }
                        dataOffset += guidSecHdrSize;
                        dataSize -= guidSecHdrSize;
                    }
                }

                // Read the raw APML blob
                std::vector<uint8_t> raw(dataSize);
                rom.seekg(static_cast<std::streamoff>(dataOffset));
                rom.read(reinterpret_cast<char*>(raw.data()),
                         static_cast<std::streamsize>(dataSize));

                if (!rom.good())
                {
                    lg2::error("PlatDef: read failed at offset 0x{OFF}",
                               "OFF", lg2::hex, dataOffset);
                    return {};
                }

                lg2::info("PlatDef: read {SZ} bytes from ROM", "SZ",
                          raw.size());

                return decompressPlatDef(raw);
            }

            fileOffset += fileSize;
        }
    }

    lg2::info("PlatDef: APML firmware volume not found in ROM");
    return {};
}

// ---------------------------------------------------------------------------
// PlatDef record parser — extract I2C segment→CPLD mappings
//
// Records are walked using hdr.size * 16 as the stride (each record's
// size field is in units of 16 bytes). We only parse RecordType_I2CEngine
// (type 14) records; all others are skipped.
// ---------------------------------------------------------------------------

static constexpr uint8_t recordTypeI2CEngine = 14;
static constexpr uint8_t recordTypeEndOfTable = 255;
static constexpr uint8_t i2cMuxTypeCpld = 1;

std::unordered_map<uint8_t, I2cSegmentMapping>
    parseI2cSegments(const std::vector<uint8_t>& platDef)
{
    std::unordered_map<uint8_t, I2cSegmentMapping> result;

    if (platDef.size() < sizeof(PlatDefRecordHeader))
    {
        return result;
    }

    // Validate Gen11 record layout: the first record is the table data
    // header whose stride should match sizeof(PlatDefTableData).
    PlatDefRecordHeader firstHdr{};
    std::memcpy(&firstHdr, platDef.data(), sizeof(firstHdr));
    if (static_cast<size_t>(firstHdr.size) * 16 != sizeof(PlatDefTableData))
    {
        lg2::error("PlatDef: unexpected table record size {SZ} "
                   "(expected {EXP}), layout may not be Gen11",
                   "SZ", static_cast<size_t>(firstHdr.size) * 16,
                   "EXP", sizeof(PlatDefTableData));
        return result;
    }

    size_t pos = 0;

    while (pos + sizeof(PlatDefRecordHeader) <= platDef.size())
    {
        PlatDefRecordHeader hdr{};
        std::memcpy(&hdr, platDef.data() + pos, sizeof(hdr));

        if (hdr.type == recordTypeEndOfTable || hdr.size == 0)
        {
            break;
        }

        size_t recordBytes = static_cast<size_t>(hdr.size) * 16;

        if (pos + recordBytes > platDef.size())
        {
            lg2::warning("PlatDef: record at 0x{OFF} claims {SZ} bytes, "
                         "only {REM} remain",
                         "OFF", lg2::hex, pos, "SZ", recordBytes,
                         "REM", platDef.size() - pos);
            break;
        }

        if (hdr.type == recordTypeI2CEngine)
        {
            size_t fixedEnd = pos + sizeof(PlatDefRecordHeader) +
                              sizeof(PlatDefI2CEngineFixed);
            if (fixedEnd > platDef.size())
            {
                break;
            }

            PlatDefI2CEngineFixed eng{};
            std::memcpy(&eng, platDef.data() + pos +
                                  sizeof(PlatDefRecordHeader),
                        sizeof(eng));

            size_t recordEnd = pos + recordBytes;
            size_t segStart = fixedEnd;

            for (uint8_t i = 0; i < eng.count; i++)
            {
                size_t segOff = segStart + i * sizeof(PlatDefI2CSegment);
                if (segOff + sizeof(PlatDefI2CSegment) > platDef.size() ||
                    segOff + sizeof(PlatDefI2CSegment) > recordEnd)
                {
                    break;
                }

                PlatDefI2CSegment seg{};
                std::memcpy(&seg, platDef.data() + segOff, sizeof(seg));

                if (seg.muxControl.muxType == i2cMuxTypeCpld)
                {
                    result[seg.id] = {seg.muxControl.cpld.byte,
                                      seg.muxControl.cpld.selectMask};

                    lg2::debug("PlatDef I2C: engine={ENG} seg=0x{SEG} "
                               "cpld=0x{CPLD} chan={CH}",
                               "ENG", eng.id, "SEG", lg2::hex, seg.id,
                               "CPLD", lg2::hex, seg.muxControl.cpld.byte,
                               "CH", seg.muxControl.cpld.selectMask);
                }
            }
        }

        pos += recordBytes;
    }

    lg2::info("PlatDef: parsed {CNT} I2C segment mappings",
              "CNT", result.size());
    return result;
}

} // namespace chif
