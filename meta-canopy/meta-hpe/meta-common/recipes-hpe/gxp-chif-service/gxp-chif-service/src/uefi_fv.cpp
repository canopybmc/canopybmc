// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH

// Minimal UEFI firmware volume parser. Scans a flash image for FFS2
// firmware volumes and extracts file payloads by GUID. Only handles
// the subset of UEFI PI structures needed for PlatDef extraction.

#include "uefi_fv.hpp"

#include <phosphor-logging/lg2.hpp>

#include <cstring>

namespace chif
{

// Internal UEFI packed structures — not exposed in the header.
#pragma pack(push, 1)

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

#pragma pack(pop)

static constexpr uint32_t efiFvhSignature = 0x4856465F; // '_FVH'
static constexpr uint8_t efiFvFileTypeFreeform = 0x02;
static constexpr uint8_t efiFfsLargeFile = 0x01;
static constexpr uint8_t efiSectionRaw = 0x19;
static constexpr uint8_t efiSectionGuidDefined = 0x02;

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

std::vector<uint8_t> extractFfsFile(std::istream& rom, size_t romSize,
                                     const EfiGuid& fvGuid,
                                     const EfiGuid& fileGuid)
{
    size_t fvOffset = 0;
    while (fvOffset + sizeof(EfiFvHeader) < romSize)
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
            fvOffset += 0x10000;
            continue;
        }

        // Valid FV — compute step from fvLength, aligned up to 64k
        auto fvLen = static_cast<size_t>(fvHdr.fvLength);
        if (fvLen & 0xFFFF)
        {
            fvLen = (fvLen & ~size_t{0xFFFF}) + 0x10000;
        }
        if (fvLen == 0)
        {
            fvLen = 0x10000;
        }

        if (!guidEqual(fvHdr.fileSystemGuid, ffs2Guid))
        {
            fvOffset += fvLen;
            continue;
        }

        if (fvHdr.extHeaderOffset == 0)
        {
            fvOffset += fvLen;
            continue;
        }

        // Read extended header to check the volume name GUID
        EfiFvExtHeader extHdr{};
        rom.seekg(
            static_cast<std::streamoff>(fvOffset + fvHdr.extHeaderOffset));
        rom.read(reinterpret_cast<char*>(&extHdr), sizeof(extHdr));
        if (!rom.good())
        {
            fvOffset += fvLen;
            continue;
        }

        if (!guidEqual(extHdr.fvName, fvGuid))
        {
            fvOffset += fvLen;
            continue;
        }

        lg2::info("UEFI FV: found matching volume at offset 0x{OFF}",
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

            if (guidEqual(fileHdr.name, fileGuid) &&
                !(fileHdr.attributes & efiFfsLargeFile))
            {
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
                        lg2::error("UEFI FV: section header read failed");
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

                // Read the file payload
                std::vector<uint8_t> data(dataSize);
                rom.seekg(static_cast<std::streamoff>(dataOffset));
                rom.read(reinterpret_cast<char*>(data.data()),
                         static_cast<std::streamsize>(dataSize));

                if (!rom.good())
                {
                    lg2::error("UEFI FV: read failed at offset 0x{OFF}",
                               "OFF", lg2::hex, dataOffset);
                    return {};
                }

                lg2::info("UEFI FV: extracted {SZ} bytes", "SZ",
                          data.size());
                return data;
            }

            fileOffset += fileSize;
        }

        fvOffset += fvLen;
    }

    return {};
}

} // namespace chif
