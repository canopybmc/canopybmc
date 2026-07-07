// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#pragma once

#include <sdbusplus/bus.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace chif
{

enum class AddrOrigin : uint8_t
{
    stat = 0,
    dhcp = 1,
    slaac = 2,
    linkLocal = 3,
};

struct Ipv6Entry
{
    std::array<uint8_t, 16> addr{};
    uint8_t prefix = 0;
    AddrOrigin origin = AddrOrigin::stat;
};

struct NetworkConfig
{
    bool present = false;

    std::array<uint8_t, 6> mac{};
    std::string hostName;
    std::string domainName;

    bool haveV4 = false;
    bool v4Dhcp = false;
    uint32_t v4addr = 0;
    uint32_t v4mask = 0;
    uint32_t v4gateway = 0;
    std::vector<uint32_t> v4dns;

    bool v6Dhcp = false;
    bool v6AcceptRa = false;
    bool haveV6Gateway = false;
    std::array<uint8_t, 16> v6gateway{};
    std::vector<Ipv6Entry> v6addrs;
    std::vector<std::array<uint8_t, 16>> v6dns;
};

NetworkConfig queryNetworkConfig(sdbusplus::bus_t& bus,
                                 std::string_view interface = "eth0");

} // namespace chif
