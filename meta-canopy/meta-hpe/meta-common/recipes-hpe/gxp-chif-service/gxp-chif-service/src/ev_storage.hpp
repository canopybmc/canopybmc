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

// On-disk limits
inline constexpr size_t maxEvFileSize = 65536;
inline constexpr size_t maxEvNameLen = 32;
inline constexpr size_t maxEvDataSize = 3966;
inline constexpr uint32_t evFileMagic = 0x31535645; // "EVS1" LE

struct EvEntry
{
    std::string name;
    std::vector<uint8_t> data;
};

// ---------------------------------------------------------------------------
// EvStorage — persistent key-value store for BIOS environment variables.
//
// File format (little-endian):
//   [4B magic "EVS1"][4B count]
//   Repeated count times:
//     [32B name (null-padded)][2B dataLen][dataLen bytes]
//
// All mutations persist to disk immediately via atomic write (write to
// .tmp then rename).  Thread safety is not needed — the CHIF daemon is
// single-threaded.
// ---------------------------------------------------------------------------
class EvStorage
{
  public:
    explicit EvStorage(
        std::filesystem::path path = "/var/lib/chif/evs.dat");

    int load();

    std::optional<EvEntry> getByIndex(uint32_t index) const;
    std::optional<EvEntry> getByName(const std::string& name) const;

    bool set(const std::string& name, std::span<const uint8_t> data);
    bool del(const std::string& name);
    bool deleteAll();

    uint32_t count() const;
    size_t remainingSize() const;

    static constexpr size_t maxSize()
    {
        return maxEvFileSize;
    }

  private:
    bool save();
    size_t serializedSize() const;

    std::filesystem::path path_;
    std::vector<EvEntry> entries_;
};

} // namespace chif
