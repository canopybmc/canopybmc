// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace chif
{

// MDR V2 header written at the start of /var/lib/smbios/smbios2.
// Must match smbios-mdr's MDRSMBIOSHeader exactly (10 bytes).
struct MdrHeader
{
    uint8_t dirVer;
    uint8_t mdrType;
    uint32_t timestamp;
    uint32_t dataSize;
} __attribute__((packed));

static_assert(sizeof(MdrHeader) == 10, "MdrHeader must be 10 bytes");

// SMBIOS 3.0 Entry Point (24 bytes), placed immediately after MDR header.
struct Smbios3EntryPoint
{
    uint8_t anchorString[5]; // "_SM3_"
    uint8_t checksum;
    uint8_t entryPointLength;
    uint8_t smbiosMajor;
    uint8_t smbiosMinor;
    uint8_t smbiosDocrev;
    uint8_t entryPointRevision;
    uint8_t reserved;
    uint32_t structureTableMaxSize;
    uint64_t structureTableAddress;
} __attribute__((packed));

static_assert(sizeof(Smbios3EntryPoint) == 24,
              "Smbios3EntryPoint must be 24 bytes");

// Safety limits for host-provided data
inline constexpr size_t maxSmbiosDataSize = 1024 * 1024; // 1 MiB
inline constexpr uint32_t maxRecordsPerBlob = 1000;

// Accumulates SMBIOS records and writes the final smbios2 file
// with MDR header + SMBIOS 3.0 Entry Point + raw records.
class SmbiosWriter
{
  public:
    // Directory where smbios files live (default: /var/lib/smbios/)
    explicit SmbiosWriter(
        std::filesystem::path dir = "/var/lib/smbios");

    // Begin a new SMBIOS session — clears any accumulated records.
    void begin();

    // Append a single SMBIOS record (raw bytes including type/length/handle
    // header and trailing double-null terminator).
    void addRecord(std::span<const uint8_t> record);

    // Finalize: construct SMBIOS 3.0 EP + MDR header, write the smbios2
    // file atomically (write temp, rename). Returns true on success.
    bool finalize();

    // Get the total accumulated data size (all records).
    size_t dataSize() const;

    // Get the path to the final smbios2 file.
    std::filesystem::path outputPath() const;

  private:
    std::filesystem::path dir_;
    std::vector<uint8_t> records_;
    uint8_t directoryVersion_{0};

    // Compute the SMBIOS 3.0 EP checksum.
    static uint8_t computeChecksum(const Smbios3EntryPoint& ep);
};

} // namespace chif
