// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH

#include "ev_storage.hpp"

#include <phosphor-logging/lg2.hpp>

#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <fstream>

namespace chif
{

// File header size: magic(4) + count(4)
static constexpr size_t fileHeaderSize = 8;

// Per-entry overhead: name(32) + dataLen(2)
static constexpr size_t entryOverhead = maxEvNameLen + sizeof(uint16_t);

EvStorage::EvStorage(std::filesystem::path path) : path_(std::move(path)) {}

bool EvStorage::load()
{
    if (!std::filesystem::exists(path_))
    {
        entries_.clear();
        lg2::info("EV: no file at {PATH}, starting fresh", "PATH",
                  path_.string());
        return true;
    }

    std::ifstream in(path_, std::ios::binary);
    if (!in)
    {
        lg2::error("EV: failed to open {PATH}", "PATH", path_.string());
        return false;
    }

    uint32_t magic = 0;
    uint32_t cnt = 0;
    in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    in.read(reinterpret_cast<char*>(&cnt), sizeof(cnt));

    if (!in || magic != evFileMagic)
    {
        lg2::error("EV: bad magic 0x{M:08x} in {PATH}", "M", magic, "PATH",
                   path_.string());
        entries_.clear();
        return false;
    }

    // Sanity limit — even at 34 bytes per entry, 64 KB holds < 2000
    if (cnt > 10000)
    {
        lg2::error("EV: unreasonable count {C}", "C", cnt);
        entries_.clear();
        return false;
    }

    entries_.clear();
    entries_.reserve(cnt);

    for (uint32_t i = 0; i < cnt; i++)
    {
        char nameBuf[maxEvNameLen] = {};
        in.read(nameBuf, sizeof(nameBuf));

        uint16_t dataLen = 0;
        in.read(reinterpret_cast<char*>(&dataLen), sizeof(dataLen));

        if (!in || dataLen > maxEvDataSize)
        {
            lg2::error("EV: corrupted entry at index {I}", "I", i);
            break;
        }

        std::vector<uint8_t> data(dataLen);
        if (dataLen > 0)
        {
            in.read(reinterpret_cast<char*>(data.data()), dataLen);
        }

        if (!in)
        {
            lg2::error("EV: truncated entry at index {I}", "I", i);
            break;
        }

        EvEntry entry;
        entry.name =
            std::string(nameBuf, strnlen(nameBuf, sizeof(nameBuf)));
        entry.data = std::move(data);
        entries_.push_back(std::move(entry));
    }

    lg2::info("EV: loaded {N} entries from {PATH}", "N",
              static_cast<uint32_t>(entries_.size()), "PATH",
              path_.string());
    return true;
}

bool EvStorage::save()
{
    auto dir = path_.parent_path();
    if (!dir.empty())
    {
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        if (ec)
        {
            lg2::error("EV: failed to create directory {DIR}: {ERR}", "DIR",
                       dir.string(), "ERR", ec.message());
            return false;
        }
    }

    auto tempPath = path_;
    tempPath += ".tmp";

    std::ofstream out(tempPath, std::ios::binary | std::ios::trunc);
    if (!out)
    {
        lg2::error("EV: failed to create {PATH}", "PATH",
                   tempPath.string());
        return false;
    }

    // Write header
    uint32_t magic = evFileMagic;
    uint32_t cnt = static_cast<uint32_t>(entries_.size());
    out.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    out.write(reinterpret_cast<const char*>(&cnt), sizeof(cnt));

    // Write entries
    for (const auto& entry : entries_)
    {
        char nameBuf[maxEvNameLen] = {};
        std::strncpy(nameBuf, entry.name.c_str(), sizeof(nameBuf) - 1);
        out.write(nameBuf, sizeof(nameBuf));

        uint16_t dataLen = static_cast<uint16_t>(entry.data.size());
        out.write(reinterpret_cast<const char*>(&dataLen), sizeof(dataLen));

        if (dataLen > 0)
        {
            out.write(
                reinterpret_cast<const char*>(entry.data.data()), dataLen);
        }
    }

    out.flush();
    if (!out)
    {
        lg2::error("EV: write failed for {PATH}", "PATH",
                   tempPath.string());
        std::filesystem::remove(tempPath);
        return false;
    }

    // Ensure data reaches persistent storage before rename
    {
        int fd = ::open(tempPath.c_str(), O_RDONLY);
        if (fd >= 0)
        {
            ::fsync(fd);
            ::close(fd);
        }
    }

    out.close();

    // Atomic rename
    std::error_code ec;
    std::filesystem::rename(tempPath, path_, ec);
    if (ec)
    {
        lg2::error("EV: rename failed: {ERR}", "ERR", ec.message());
        std::filesystem::remove(tempPath);
        return false;
    }

    return true;
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
    for (const auto& entry : entries_)
    {
        if (entry.name == name)
        {
            return entry;
        }
    }
    return std::nullopt;
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

    // Check if updating an existing entry
    for (auto& entry : entries_)
    {
        if (entry.name == name)
        {
            entry.data.assign(data.begin(), data.end());
            return save();
        }
    }

    // New entry — check space
    size_t newEntrySize = entryOverhead + data.size();
    if (serializedSize() + newEntrySize > maxEvFileSize)
    {
        lg2::warning("EV: no space for new entry {NAME} ({SZ} bytes)",
                     "NAME", name, "SZ", data.size());
        return false;
    }

    EvEntry entry;
    entry.name = name;
    entry.data.assign(data.begin(), data.end());
    entries_.push_back(std::move(entry));

    return save();
}

bool EvStorage::del(const std::string& name)
{
    auto it = std::find_if(
        entries_.begin(), entries_.end(),
        [&name](const EvEntry& e) { return e.name == name; });

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
    return used < maxEvFileSize ? maxEvFileSize - used : 0;
}

size_t EvStorage::serializedSize() const
{
    size_t size = fileHeaderSize;
    for (const auto& entry : entries_)
    {
        size += entryOverhead + entry.data.size();
    }
    return size;
}

} // namespace chif
