// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#include "net_config.hpp"

#include "utils.hpp"

#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/Network/EthernetInterface/common.hpp>
#include <xyz/openbmc_project/Network/IP/common.hpp>
#include <xyz/openbmc_project/Network/MACAddress/common.hpp>
#include <xyz/openbmc_project/Network/SystemConfiguration/common.hpp>
#include <xyz/openbmc_project/ObjectMapper/common.hpp>

#include <format>
#include <map>
#include <optional>
#include <variant>

namespace chif
{

namespace
{

namespace network = sdbusplus::common::xyz::openbmc_project::network;
using EthernetInterface = network::EthernetInterface;
using MACAddress = network::MACAddress;
using IP = network::IP;
using SystemConfiguration = network::SystemConfiguration;
using ObjectMapper = sdbusplus::common::xyz::openbmc_project::ObjectMapper;

constexpr auto networkService = "xyz.openbmc_project.Network";
constexpr auto networkRoot = "/xyz/openbmc_project/network";
constexpr auto sysConfigPath = "/xyz/openbmc_project/network/config";

AddrOrigin mapOrigin(IP::AddressOrigin o)
{
    switch (o)
    {
        case IP::AddressOrigin::DHCP:
            return AddrOrigin::dhcp;
        case IP::AddressOrigin::SLAAC:
            return AddrOrigin::slaac;
        case IP::AddressOrigin::LinkLocal:
            return AddrOrigin::linkLocal;
        default:
            return AddrOrigin::stat;
    }
}

std::vector<std::string> getIpPaths(sdbusplus::bus_t& bus,
                                    const std::string& ifacePath)
{
    try
    {
        auto m = bus.new_method_call(
            ObjectMapper::default_service, ObjectMapper::instance_path,
            ObjectMapper::interface,
            ObjectMapper::method_names::get_sub_tree_paths);
        m.append(ifacePath, 0, std::vector<std::string>{IP::interface});
        auto reply = bus.call(m);
        std::vector<std::string> paths;
        reply.read(paths);
        return paths;
    }
    catch (const std::exception&)
    {
        return {};
    }
}

void collectIpAddresses(sdbusplus::bus_t& bus, const std::string& ifacePath,
                        NetworkConfig& cfg)
{
    using IpVariant = std::variant<std::string, uint8_t, bool, uint32_t>;

    for (const auto& path : getIpPaths(bus, ifacePath))
    {
        std::map<std::string, IpVariant> props;
        try
        {
            auto m = bus.new_method_call(networkService, path.c_str(),
                                         propertiesInterface, "GetAll");
            m.append(IP::interface);
            bus.call(m).read(props);
        }
        catch (const std::exception&)
        {
            continue;
        }

        auto getStr = [&](const char* key) -> std::string {
            auto it = props.find(key);
            if (it == props.end())
            {
                return {};
            }
            const auto* s = std::get_if<std::string>(&it->second);
            return s ? *s : std::string{};
        };

        std::string addrStr = getStr(IP::property_names::address);
        uint8_t prefix = 0;
        if (auto it = props.find(IP::property_names::prefix_length);
            it != props.end() && std::holds_alternative<uint8_t>(it->second))
        {
            prefix = std::get<uint8_t>(it->second);
        }

        IP::Protocol proto{};
        AddrOrigin origin = AddrOrigin::stat;
        try
        {
            proto =
                IP::convertProtocolFromString(getStr(IP::property_names::type));
            origin = mapOrigin(IP::convertAddressOriginFromString(
                getStr(IP::property_names::origin)));
        }
        catch (const std::exception&)
        {
            continue;
        }

        if (proto == IP::Protocol::IPv4)
        {
            if (auto v4 = parseV4(addrStr))
            {
                // Prefer the first routable (non-link-local) address.
                if (!cfg.haveV4 || origin != AddrOrigin::linkLocal)
                {
                    cfg.haveV4 = true;
                    cfg.v4addr = *v4;
                    cfg.v4mask = prefixToMask4(prefix);
                    cfg.v4Dhcp = (origin == AddrOrigin::dhcp);
                }
            }
        }
        else if (proto == IP::Protocol::IPv6)
        {
            if (auto v6 = parseV6(addrStr))
            {
                cfg.v6addrs.push_back({*v6, prefix, origin});
            }
        }
    }
}

} // namespace

NetworkConfig queryNetworkConfig(sdbusplus::bus_t& bus,
                                 std::string_view interface)
{
    NetworkConfig cfg;
    const std::string ifacePath = std::format("{}/{}", networkRoot, interface);

    auto dhcp = getProperty<std::string>(
        bus, networkService, ifacePath.c_str(), EthernetInterface::interface,
        EthernetInterface::property_names::dhcp_enabled);
    if (!dhcp)
    {
        lg2::warning("Network interface {IF} not found on D-Bus", "IF",
                     interface);
        return cfg;
    }
    cfg.present = true;

    using DHCPConf = EthernetInterface::DHCPConf;
    auto conf = EthernetInterface::convertStringToDHCPConf(*dhcp).value_or(
        DHCPConf::none);
    cfg.v4Dhcp = conf == DHCPConf::both || conf == DHCPConf::v4 ||
                 conf == DHCPConf::v4v6stateless;
    cfg.v6Dhcp = conf == DHCPConf::both || conf == DHCPConf::v6;

    if (auto ra = getProperty<bool>(
            bus, networkService, ifacePath.c_str(),
            EthernetInterface::interface,
            EthernetInterface::property_names::i_pv6_accept_ra))
    {
        cfg.v6AcceptRa = *ra;
    }

    if (auto mac = getProperty<std::string>(
            bus, networkService, ifacePath.c_str(), MACAddress::interface,
            MACAddress::property_names::mac_address))
    {
        if (auto parsed = parseMac(*mac))
        {
            cfg.mac = *parsed;
        }
    }

    if (auto host = getProperty<std::string>(
            bus, networkService, sysConfigPath, SystemConfiguration::interface,
            SystemConfiguration::property_names::host_name))
    {
        cfg.hostName = *host;
    }

    if (auto domains = getProperty<std::vector<std::string>>(
            bus, networkService, ifacePath.c_str(),
            EthernetInterface::interface,
            EthernetInterface::property_names::domain_name);
        domains && !domains->empty())
    {
        cfg.domainName = domains->front();
    }

    if (auto gw = getProperty<std::string>(
            bus, networkService, ifacePath.c_str(),
            EthernetInterface::interface,
            EthernetInterface::property_names::default_gateway))
    {
        if (auto v4 = parseV4(*gw))
        {
            cfg.v4gateway = *v4;
        }
    }

    if (auto gw6 = getProperty<std::string>(
            bus, networkService, ifacePath.c_str(),
            EthernetInterface::interface,
            EthernetInterface::property_names::default_gateway6))
    {
        if (auto v6 = parseV6(*gw6))
        {
            cfg.v6gateway = *v6;
            cfg.haveV6Gateway = true;
        }
    }

    if (auto ns = getProperty<std::vector<std::string>>(
            bus, networkService, ifacePath.c_str(),
            EthernetInterface::interface,
            EthernetInterface::property_names::nameservers))
    {
        for (const auto& s : *ns)
        {
            if (auto v4 = parseV4(s))
            {
                cfg.v4dns.push_back(*v4);
            }
            else if (auto v6 = parseV6(s))
            {
                cfg.v6dns.push_back(*v6);
            }
        }
    }

    collectIpAddresses(bus, ifacePath, cfg);

    return cfg;
}

} // namespace chif
