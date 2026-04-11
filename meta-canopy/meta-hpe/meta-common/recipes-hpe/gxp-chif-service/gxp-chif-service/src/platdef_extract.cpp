// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH

// Parse the HPE Platform Definition (PlatDef) blob and extract I2C
// segment-to-bus mappings for the CHIF I2C proxy. The raw blob is
// extracted from the host BIOS SPI flash via the UEFI FV parser
// (uefi_fv.hpp), then decompressed and walked for I2CEngine records.

#include "platdef_extract.hpp"

#include "uefi_fv.hpp"

#include <phosphor-logging/lg2.hpp>

#include <zlib.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace chif
{

// PlatDef bundle and record structures (from HPE openbmc-chif-svc platdef.h)
#pragma pack(push, 1)

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

// APML firmware volume GUID: {7EBF5AB8-525E-417C-9B6B-5EF367856954}
static constexpr EfiGuid apmlFvGuid = {
    0x7EBF5AB8, 0x525E, 0x417C,
    {0x9B, 0x6B, 0x5E, 0xF3, 0x67, 0x85, 0x69, 0x54}};

// APML file GUID: {C5F6001C-39B4-43DD-9B9B-6832F1BB4BE9}
static constexpr EfiGuid apmlFileGuid = {
    0xC5F6001C, 0x39B4, 0x43DD,
    {0x9B, 0x9B, 0x68, 0x32, 0xF1, 0xBB, 0x4B, 0xE9}};

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

    auto raw = extractFfsFile(rom, romSize, apmlFvGuid, apmlFileGuid);
    if (raw.empty())
    {
        lg2::info("PlatDef: APML firmware volume not found in ROM");
        return {};
    }

    return decompressPlatDef(raw);
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

// ---------------------------------------------------------------------------
// Combine PlatDef segment data with sysfs i2cmux topology to build the
// final segment→kernel bus mapping.
//
// PlatDef gives us: segment → (cpldRegister, channelValue)
// sysfs gives us:   (muxIndex, channelNumber) → kernelBus
// Correlation:      muxIndex = cpldRegister - cpldRegisterBase
// ---------------------------------------------------------------------------

// GXP AHB bus path where i2cmux devices appear in sysfs
static constexpr auto ahbSysfsPath = "/sys/devices/platform/ahb@80000000";

// CPLD register base address for mux index 0. Mux N maps to register
// base + N. Derived from the device tree mux-reg-masks property.
static constexpr uint8_t cpldRegisterBase = 0x84;

// Scan sysfs for i2cmux channel→bus mappings.
// Key: (muxIndex << 8) | channelNumber — valid for channelNumber < 256.
static std::unordered_map<uint16_t, int> scanMuxTopology()
{
    std::unordered_map<uint16_t, int> result;
    const std::filesystem::path ahbPath = ahbSysfsPath;

    if (!std::filesystem::exists(ahbPath))
    {
        return result;
    }

    try
    {
        for (const auto& entry :
             std::filesystem::directory_iterator(ahbPath))
        {
            std::string name = entry.path().filename().string();
            if (!name.starts_with("ahb@") ||
                name.find("i2cmux@") == std::string::npos)
            {
                continue;
            }

            auto muxPos = name.find("i2cmux@");
            unsigned long muxVal = 0;
            try
            {
                muxVal = std::stoul(name.substr(muxPos + 7));
            }
            catch (const std::exception&)
            {
                continue;
            }
            if (muxVal > std::numeric_limits<uint8_t>::max())
            {
                continue;
            }
            auto muxId = static_cast<uint8_t>(muxVal);

            for (const auto& chanEntry :
                 std::filesystem::directory_iterator(entry.path()))
            {
                std::string chanName =
                    chanEntry.path().filename().string();
                if (!chanName.starts_with("channel-"))
                {
                    continue;
                }

                unsigned long chanVal = 0;
                try
                {
                    chanVal = std::stoul(chanName.substr(8));
                }
                catch (const std::exception&)
                {
                    continue;
                }
                if (chanVal > std::numeric_limits<uint8_t>::max())
                {
                    continue;
                }
                auto chanIdx = static_cast<uint8_t>(chanVal);

                std::error_code ec;
                auto target =
                    std::filesystem::read_symlink(chanEntry.path(), ec);
                if (ec)
                {
                    continue;
                }

                std::string targetName = target.filename().string();
                if (!targetName.starts_with("i2c-"))
                {
                    continue;
                }

                int busNum = 0;
                try
                {
                    busNum = std::stoi(targetName.substr(4));
                }
                catch (const std::exception&)
                {
                    continue;
                }

                uint16_t key =
                    (static_cast<uint16_t>(muxId) << 8) | chanIdx;
                result[key] = busNum;
            }
        }
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        lg2::warning("I2C mux scan failed: {ERR}", "ERR", e.what());
    }

    return result;
}

std::unordered_map<uint8_t, int> buildSegmentBusMap(
    const std::unordered_map<uint8_t, I2cSegmentMapping>& segments)
{
    auto muxTopology = scanMuxTopology();
    std::unordered_map<uint8_t, int> result;

    for (const auto& [segId, mapping] : segments)
    {
        if (mapping.cpldRegister < cpldRegisterBase)
        {
            continue;
        }

        uint8_t muxIndex = mapping.cpldRegister - cpldRegisterBase;
        uint16_t key = (static_cast<uint16_t>(muxIndex) << 8) |
                       mapping.channelValue;

        auto it = muxTopology.find(key);
        if (it != muxTopology.end())
        {
            result[segId] = it->second;
            lg2::debug("I2C map: seg=0x{SEG} -> mux={MUX} chan={CH} "
                       "-> i2c-{BUS}",
                       "SEG", lg2::hex, segId, "MUX", muxIndex,
                       "CH", mapping.channelValue, "BUS", it->second);
        }
        else
        {
            lg2::debug("I2C map: seg=0x{SEG} unresolved "
                       "(mux={MUX} chan={CH})",
                       "SEG", lg2::hex, segId, "MUX", muxIndex,
                       "CH", mapping.channelValue);
        }
    }

    lg2::info("I2C map: resolved {CNT} of {TOTAL} segments to kernel buses",
              "CNT", result.size(), "TOTAL", segments.size());
    return result;
}

} // namespace chif
