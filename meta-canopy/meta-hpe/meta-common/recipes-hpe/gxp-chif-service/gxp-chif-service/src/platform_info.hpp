// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#pragma once

#include <cstdint>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace chif
{

// Helpers for reading platform identity files used by SMIF command handlers.
// These are intentionally simple — read once, cache in the caller.

// Read a field from /etc/os-release (or a custom path for testing).
// The file format is KEY=VALUE or KEY="VALUE" per line.
// Returns std::nullopt if the field is not found.
inline std::optional<std::string> readOsReleaseField(
    const std::string& field,
    const std::string& path = "/etc/os-release")
{
    std::ifstream in(path);
    if (!in)
    {
        return std::nullopt;
    }

    std::string line;
    std::string prefix = field + "=";
    while (std::getline(in, line))
    {
        if (line.compare(0, prefix.size(), prefix) != 0)
        {
            continue;
        }

        std::string value = line.substr(prefix.size());

        // Strip surrounding quotes if present
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
        {
            value = value.substr(1, value.size() - 2);
        }

        return value;
    }

    return std::nullopt;
}

// Read a null-terminated string from a device-tree property file.
// Device-tree string properties include a trailing NUL byte in the file.
// Returns empty string on failure.
inline std::string readDeviceTreeString(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
    {
        return {};
    }

    std::string content((std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>());

    // Strip trailing NUL bytes (device-tree convention)
    while (!content.empty() && content.back() == '\0')
    {
        content.pop_back();
    }

    return content;
}

// Read raw binary bytes from a file (e.g., /proc/device-tree/chosen/uuid).
// Returns empty vector on failure. Reads at most maxBytes to avoid
// unbounded memory allocation from unexpected file sizes.
inline std::vector<uint8_t> readBinaryFile(const std::string& path,
                                           size_t maxBytes = 4096)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
    {
        return {};
    }

    std::vector<uint8_t> data(maxBytes);
    in.read(reinterpret_cast<char*>(data.data()),
            static_cast<std::streamsize>(maxBytes));
    data.resize(static_cast<size_t>(in.gcount()));
    return data;
}

// Parse a firmware version string like "3.0.0-dev" into a packed uint16_t.
// Format: major in high byte, minor in low byte (e.g., "3.1.0" -> 0x0301).
// Returns 0 on parse failure.
inline uint16_t parseFirmwareVersion(const std::string& versionStr)
{
    unsigned int major = 0;
    unsigned int minor = 0;

    // Try to parse "X.Y" or "X.Y.Z" or "X.Y.Z-suffix"
    std::istringstream ss(versionStr);
    char dot = 0;

    ss >> major;
    if (ss.peek() == '.')
    {
        ss >> dot >> minor;
    }

    if (major > 255)
    {
        major = 255;
    }
    if (minor > 255)
    {
        minor = 255;
    }

    return static_cast<uint16_t>((major << 8) | minor);
}

} // namespace chif
