// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#include "ev_storage.hpp"

#include <phosphor-logging/lg2.hpp>

#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>

namespace chif
{

// Per-entry overhead on disk: 32 (name) + 2 (dataLen)
static constexpr size_t entryOverhead = maxEvNameLen + sizeof(uint16_t);
// File header: 4 (magic) + 4 (count)
static constexpr size_t headerSize = sizeof(uint32_t) + sizeof(uint32_t);

EvStorage::EvStorage(std::filesystem::path path) : path_(std::move(path)) {}

int EvStorage::load()
{
    entries_.clear();

    // Ensure parent directory exists (normally created by StateDirectory=)
    auto dir = path_.parent_path();
    if (!dir.empty())
    {
        std::filesystem::create_directories(dir);
    }

    if (!std::filesystem::exists(path_))
    {
        lg2::info("EV storage: no file at {PATH}, starting fresh", "PATH",
                  path_.string());
        return 0;
    }

    std::ifstream file(path_, std::ios::binary);
    if (!file)
    {
        lg2::error("EV storage: failed to open {PATH}", "PATH",
                   path_.string());
        return -1;
    }

    uint32_t magic = 0;
    uint32_t count = 0;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    file.read(reinterpret_cast<char*>(&count), sizeof(count));

    if (magic != evFileMagic)
    {
        lg2::error("EV storage: bad magic {MAGIC} in {PATH}", "MAGIC",
                   lg2::hex, magic, "PATH", path_.string());
        return -1;
    }

    if (count > 10000)
    {
        lg2::error("EV storage: unreasonable count {COUNT}", "COUNT", count);
        return -1;
    }

    size_t totalRead = headerSize;
    for (uint32_t i = 0; i < count && file.good(); i++)
    {
        char nameBuf[maxEvNameLen] = {};
        file.read(nameBuf, sizeof(nameBuf));

        uint16_t dataLen = 0;
        file.read(reinterpret_cast<char*>(&dataLen), sizeof(dataLen));

        if (dataLen > maxEvDataSize ||
            totalRead + entryOverhead + dataLen > maxEvFileSize ||
            !file.good())
        {
            lg2::warning("EV storage: truncated at entry {IDX}", "IDX", i);
            break;
        }
        totalRead += entryOverhead + dataLen;

        std::vector<uint8_t> data(dataLen);
        if (dataLen > 0)
        {
            file.read(reinterpret_cast<char*>(data.data()),
                      static_cast<std::streamsize>(dataLen));
        }

        if (!file.good())
        {
            lg2::warning("EV storage: truncated at entry {IDX}", "IDX", i);
            break;
        }

        nameBuf[maxEvNameLen - 1] = '\0';
        entries_.push_back({std::string(nameBuf), std::move(data)});
    }

    lg2::info("EV storage: loaded {COUNT} entries from {PATH}", "COUNT",
              entries_.size(), "PATH", path_.string());
    return 0;
}

std::optional<EvEntry> EvStorage::getByIndex(uint32_t index) const
{
    if (index >= entries_.size())
    {
        return std::nullopt;
    }
    return entries_[index];
}

std::optional<EvEntry> EvStorage::getByName(const std::string& name) const
{
    auto it = std::find_if(entries_.begin(), entries_.end(),
                           [&](const EvEntry& e) { return e.name == name; });
    if (it == entries_.end())
    {
        return std::nullopt;
    }
    return *it;
}

bool EvStorage::set(const std::string& name, std::span<const uint8_t> data)
{
    if (name.empty() || name.size() >= maxEvNameLen)
    {
        return false;
    }
    if (data.size() > maxEvDataSize)
    {
        return false;
    }

    // Check if entry already exists — update in place
    auto it = std::find_if(entries_.begin(), entries_.end(),
                           [&](const EvEntry& e) { return e.name == name; });
    if (it != entries_.end())
    {
        // Verify the size delta still fits within the file limit
        if (data.size() > it->data.size())
        {
            size_t delta = data.size() - it->data.size();
            if (serializedSize() + delta > maxEvFileSize)
            {
                lg2::warning(
                    "EV storage: update would exceed max size for {NAME}",
                    "NAME", name);
                return false;
            }
        }
        it->data.assign(data.begin(), data.end());
        return save();
    }

    // New entry — check space
    size_t needed = entryOverhead + data.size();
    if (serializedSize() + needed > maxEvFileSize)
    {
        lg2::warning("EV storage: no space for {NAME} ({NEEDED} bytes)",
                     "NAME", name, "NEEDED", needed);
        return false;
    }

    entries_.push_back(
        {name, std::vector<uint8_t>(data.begin(), data.end())});
    return save();
}

bool EvStorage::del(const std::string& name)
{
    auto it = std::find_if(entries_.begin(), entries_.end(),
                           [&](const EvEntry& e) { return e.name == name; });
    if (it == entries_.end())
    {
        return false;
    }

    entries_.erase(it);
    return save();
}

bool EvStorage::deleteAll()
{
    entries_.clear();
    return save();
}

uint32_t EvStorage::count() const
{
    return static_cast<uint32_t>(entries_.size());
}

size_t EvStorage::remainingSize() const
{
    size_t used = serializedSize();
    return (used < maxEvFileSize) ? (maxEvFileSize - used) : 0;
}

size_t EvStorage::serializedSize() const
{
    size_t total = headerSize;
    for (const auto& entry : entries_)
    {
        total += entryOverhead + entry.data.size();
    }
    return total;
}

bool EvStorage::save()
{
    auto tmpPath = std::filesystem::path(std::tmpnam(nullptr));

    std::ofstream file(tmpPath, std::ios::binary | std::ios::trunc);
    if (!file)
    {
        lg2::error("EV storage: failed to create {PATH}", "PATH",
                   tmpPath.string());
        return false;
    }

    // Header
    uint32_t magic = evFileMagic;
    uint32_t cnt = static_cast<uint32_t>(entries_.size());
    file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    file.write(reinterpret_cast<const char*>(&cnt), sizeof(cnt));

    // Entries
    for (const auto& entry : entries_)
    {
        char nameBuf[maxEvNameLen] = {};
        std::strncpy(nameBuf, entry.name.c_str(), maxEvNameLen - 1);
        file.write(nameBuf, sizeof(nameBuf));

        auto dataLen = static_cast<uint16_t>(entry.data.size());
        file.write(reinterpret_cast<const char*>(&dataLen), sizeof(dataLen));

        if (dataLen > 0)
        {
            file.write(reinterpret_cast<const char*>(entry.data.data()),
                       static_cast<std::streamsize>(dataLen));
        }
    }

    file.flush();
    if (!file.good())
    {
        lg2::error("EV storage: write failed for {PATH}", "PATH",
                   tmpPath.string());
        std::filesystem::remove(tmpPath);
        return false;
    }
    file.close();

    // Copy from tmpfs to rwfs (cross-device rename is not possible)
    std::error_code ec;
    std::filesystem::copy_file(tmpPath, path_,
                               std::filesystem::copy_options::overwrite_existing,
                               ec);
    std::filesystem::remove(tmpPath);
    if (ec)
    {
        lg2::error("EV storage: copy to {PATH} failed: {ERR}", "PATH",
                   path_.string(), "ERR", ec.message());
        return false;
    }

    return true;
}

} // namespace chif
