// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#include "utils.hpp"

#include <arpa/inet.h>

#include <algorithm>
#include <charconv>

namespace chif
{

std::optional<uint32_t> parseV4(const std::string& s)
{
    uint32_t out = 0;
    if (!s.empty() && inet_pton(AF_INET, s.c_str(), &out) == 1)
    {
        return out;
    }
    return std::nullopt;
}

std::optional<std::array<uint8_t, 16>> parseV6(const std::string& s)
{
    std::array<uint8_t, 16> out{};
    if (!s.empty() && inet_pton(AF_INET6, s.c_str(), out.data()) == 1)
    {
        return out;
    }
    return std::nullopt;
}

std::optional<std::array<uint8_t, 6>> parseMac(std::string_view s)
{
    std::array<uint8_t, 6> out{};
    for (uint8_t& byte : out)
    {
        if (s.size() < 2)
        {
            return std::nullopt;
        }
        auto [ptr, ec] = std::from_chars(s.data(), s.data() + 2, byte, 16);
        if (ec != std::errc{} || ptr != s.data() + 2)
        {
            return std::nullopt;
        }
        s.remove_prefix(2);

        // Every octet but the last is followed by a ':' separator.
        if (!s.empty() && s.front() == ':')
        {
            s.remove_prefix(1);
        }
    }
    return s.empty() ? std::optional{out} : std::nullopt;
}

uint32_t prefixToMask4(uint8_t prefix)
{
    prefix = std::min<uint8_t>(prefix, 32);
    return htonl(static_cast<uint32_t>(~uint64_t{0} << (32 - prefix)));
}

} // namespace chif
