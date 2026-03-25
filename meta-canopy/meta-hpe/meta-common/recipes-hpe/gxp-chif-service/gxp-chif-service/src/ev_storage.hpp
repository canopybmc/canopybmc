// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace chif
{

// Maximum EV storage file size (64 KB, matches HPE reference)
inline constexpr size_t maxEvFileSize = 64 * 1024;

// Maximum EV name length (including null terminator)
inline constexpr size_t maxEvNameLen = 32;

// Maximum EV data size per entry
inline constexpr size_t maxEvDataSize = 3966;

// File magic number: "EVS1" in little-endian
inline constexpr uint32_t evFileMagic = 0x31535645;

// A single environment variable entry.
struct EvEntry
{
    std::string name;
    std::vector<uint8_t> data;
};

// Persistent key-value store for BIOS environment variables.
//
// The BIOS uses SMIF commands 0x012B/0x012C/0x0130 to store and retrieve
// configuration settings (boot order, security config, etc.) that must
// survive BIOS updates. The file format is a simple flat binary:
//
//   Header (8 bytes):
//     uint32_t magic   = 0x31535645 ("EVS1")
//     uint32_t count   = number of entries
//
//   Entry (variable):
//     char name[32]     = null-padded name
//     uint16_t dataLen  = data size
//     uint8_t data[dataLen] = entry data
//
// All writes are atomic (temp file + rename).
class EvStorage
{
  public:
    // Path to the backing file. Uses /var/lib/chif/ which is on the
    // persistent JFFS2 overlay, surviving reboots and firmware updates.
    // The systemd unit uses StateDirectory=chif to create the directory.
    explicit EvStorage(
        std::filesystem::path path = "/var/lib/chif/evs.dat");

    // Load from disk. Returns true if loaded (or file didn't exist yet).
    bool load();

    // Get EV by 0-based index. Returns nullopt if out of range.
    std::optional<EvEntry> getByIndex(uint32_t index) const;

    // Get EV by name. Returns nullopt if not found.
    std::optional<EvEntry> getByName(const std::string& name) const;

    // Set (create or update) an EV. Saves to disk. Returns true on success.
    bool set(const std::string& name, std::span<const uint8_t> data);

    // Delete an EV by name. Saves to disk. Returns true if found and deleted.
    bool del(const std::string& name);

    // Delete all EVs. Saves to disk. Returns true on success.
    bool deleteAll();

    // Number of stored EVs.
    uint32_t count() const;

    // Remaining storage space in bytes.
    size_t remainingSize() const;

    // Maximum storage size.
    static constexpr size_t maxSize()
    {
        return maxEvFileSize;
    }

    // Path to the backing file.
    const std::filesystem::path& filePath() const
    {
        return path_;
    }

  private:
    std::filesystem::path path_;
    std::vector<EvEntry> entries_;

    // Save to disk atomically (temp + rename).
    bool save();

    // Compute total serialized size of header + all entries.
    size_t serializedSize() const;
};

} // namespace chif
