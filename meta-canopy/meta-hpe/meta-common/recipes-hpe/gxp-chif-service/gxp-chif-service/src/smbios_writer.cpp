// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#include "smbios_writer.hpp"

#include <phosphor-logging/lg2.hpp>

#include <chrono>
#include <cstring>
#include <fstream>

namespace chif
{

SmbiosWriter::SmbiosWriter(std::filesystem::path dir) : dir_(std::move(dir)) {}

void SmbiosWriter::begin()
{
    records_.clear();
    lg2::info("SMBIOS reception started");
}

void SmbiosWriter::addRecord(std::span<const uint8_t> record)
{
    // HPE ROM blob format: uint32 count, then count × (uint16 size + SMBIOS record).
    // Strip the wrapper and extract raw SMBIOS records for smbios-mdr.
    if (record.size() < 6)
    {
        return; // too small for even one record
    }

    uint32_t count = 0;
    std::memcpy(&count, record.data(), sizeof(count));

    if (count > maxRecordsPerBlob)
    {
        lg2::warning("Blob record count {CNT} exceeds limit {MAX}, capping",
                     "CNT", count, "MAX", maxRecordsPerBlob);
        count = maxRecordsPerBlob;
    }

    size_t offset = sizeof(count);
    for (uint32_t i = 0; i < count && offset + 2 <= record.size(); i++)
    {
        uint16_t recSize = 0;
        std::memcpy(&recSize, record.data() + offset, sizeof(recSize));
        offset += sizeof(recSize);

        if (recSize == 0 || offset + recSize > record.size())
        {
            break;
        }

        if (records_.size() + recSize > maxSmbiosDataSize)
        {
            lg2::warning(
                "SMBIOS data limit reached ({MAX} bytes), "
                "discarding further records",
                "MAX", maxSmbiosDataSize);
            break;
        }

        records_.insert(records_.end(), record.data() + offset,
                        record.data() + offset + recSize);
        offset += recSize;
    }
}

size_t SmbiosWriter::dataSize() const
{
    return records_.size();
}

std::filesystem::path SmbiosWriter::outputPath() const
{
    return dir_ / "smbios2";
}

uint8_t SmbiosWriter::computeChecksum(const Smbios3EntryPoint& ep)
{
    // Checksum: all bytes of the EP must sum to zero (mod 256).
    // We compute the checksum field as -(sum of all other bytes).
    const auto* bytes = reinterpret_cast<const uint8_t*>(&ep);
    uint8_t sum = 0;
    for (size_t i = 0; i < sizeof(ep); i++)
    {
        if (i == offsetof(Smbios3EntryPoint, checksum))
        {
            continue; // skip the checksum field itself
        }
        sum += bytes[i];
    }
    return static_cast<uint8_t>(-sum);
}

bool SmbiosWriter::finalize()
{
    if (records_.empty())
    {
        lg2::warning("SMBIOS finalize called with no records");
        return false;
    }

    // Ensure the output directory exists
    std::error_code ec;
    std::filesystem::create_directories(dir_, ec);
    if (ec)
    {
        lg2::error("Failed to create SMBIOS directory {DIR}: {ERR}", "DIR",
                   dir_.string(), "ERR", ec.message());
        return false;
    }

    // Build SMBIOS 3.0 Entry Point
    Smbios3EntryPoint ep{};
    ep.anchorString[0] = '_';
    ep.anchorString[1] = 'S';
    ep.anchorString[2] = 'M';
    ep.anchorString[3] = '3';
    ep.anchorString[4] = '_';
    ep.entryPointLength = sizeof(Smbios3EntryPoint); // 0x18
    ep.smbiosMajor = 3;
    ep.smbiosMinor = 3;
    ep.smbiosDocrev = 0;
    ep.entryPointRevision = 0x01;
    ep.reserved = 0;
    ep.structureTableMaxSize = static_cast<uint32_t>(records_.size());
    ep.structureTableAddress = 0; // not used by smbios-mdr (file-based)
    ep.checksum = computeChecksum(ep);

    // Build MDR header
    directoryVersion_++;
    auto now = std::chrono::system_clock::now();
    auto epoch = std::chrono::duration_cast<std::chrono::seconds>(
                     now.time_since_epoch())
                     .count();

    MdrHeader mdr{};
    mdr.dirVer = directoryVersion_;
    mdr.mdrType = 2;
    mdr.timestamp = static_cast<uint32_t>(epoch);
    mdr.dataSize =
        static_cast<uint32_t>(sizeof(Smbios3EntryPoint) + records_.size());

    // Write to temp file, then atomic rename
    auto tempPath = dir_ / "smbios_temp";
    auto finalPath = outputPath();

    {
        std::ofstream out(tempPath, std::ios::binary | std::ios::trunc);
        if (!out)
        {
            lg2::error("Failed to open temp file {PATH}", "PATH",
                       tempPath.string());
            return false;
        }

        out.write(reinterpret_cast<const char*>(&mdr), sizeof(mdr));
        out.write(reinterpret_cast<const char*>(&ep), sizeof(ep));
        out.write(reinterpret_cast<const char*>(records_.data()),
                  static_cast<std::streamsize>(records_.size()));

        if (!out)
        {
            lg2::error("Failed to write SMBIOS data to {PATH}", "PATH",
                       tempPath.string());
            return false;
        }
    }

    // Atomic rename
    std::filesystem::rename(tempPath, finalPath, ec);
    if (ec)
    {
        lg2::error("Failed to rename {SRC} to {DST}: {ERR}", "SRC",
                   tempPath.string(), "DST", finalPath.string(), "ERR",
                   ec.message());
        return false;
    }

    lg2::info(
        "SMBIOS data written: {SIZE} bytes, dir_version={VER}", "SIZE",
        sizeof(mdr) + sizeof(ep) + records_.size(), "VER", directoryVersion_);

    return true;
}

} // namespace chif
