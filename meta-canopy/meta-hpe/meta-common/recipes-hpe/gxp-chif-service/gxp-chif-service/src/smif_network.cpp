// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#include "net_config.hpp"
#include "net_packets.hpp"
#include "smif_service.hpp"

#include <algorithm>
#include <bit>
#include <iterator>
#include <string>

namespace chif
{

namespace
{

NetworkConfig query(sdbusplus::bus_t* bus, const std::string& iface)
{
    return bus ? queryNetworkConfig(*bus, iface) : NetworkConfig{};
}

} // namespace

int SmifService::handleIPv4Config(const ChifPktHeader& hdr,
                                  std::span<uint8_t> response)
{
    const NetworkConfig net = query(bus_, netInterface_);

    Pkt8006 msg{};
    IodCfg& cfg = msg.cfg;
    cfg.magic = netCfgMagic;
    cfg.version = netCfgVersion;

    IodIntf& intf = cfg.iface[0];
    intf.settings = netSetIpv4Enabled;
    if (!net.v6addrs.empty() || net.v6AcceptRa || net.v6Dhcp)
    {
        intf.settings |= netSetIpv6Enabled;
    }

    if (net.haveV4)
    {
        intf.ipaddr = std::byteswap(net.v4addr);
        intf.ip_mask = std::byteswap(net.v4mask);
        intf.gateway_ip = std::byteswap(net.v4gateway);
        intf.status = netStatIpAddr | netStatGateway;

        for (size_t i = 0; i < net.v4dns.size() && i < std::size(intf.dns_ip);
             ++i)
        {
            intf.dns_ip[i] = std::byteswap(net.v4dns[i]);
        }
        if (!net.v4dns.empty())
        {
            intf.status |= netStatDns;
        }

        if (net.v4Dhcp)
        {
            intf.dhcp_options = netDhcpEnabled;
            intf.dhcp_status = netDhcpStatIpAddr;
        }
    }

    // std::string::copy truncates to the field size.
    net.hostName.copy(intf.host_name, sizeof(intf.host_name) - 1);
    net.domainName.copy(intf.domain_name, sizeof(intf.domain_name) - 1);

    return emitResponse(hdr, response, msg);
}

int SmifService::handleNicConfig(const ChifPktHeader& hdr,
                                 std::span<uint8_t> response)
{
    const NetworkConfig net = query(bus_, netInterface_);

    Pkt8063 msg{};
    msg.nic_settings = nicSettingsDefault;

    std::ranges::copy(net.mac, msg.nic_mac_addr);

    if (net.haveV4)
    {
        msg.nic_ipaddr = std::byteswap(net.v4addr);
        msg.nic_ip_mask = std::byteswap(net.v4mask);
        msg.nic_gateway_ip = std::byteswap(net.v4gateway);
        msg.nic_status = netStatIpAddr | netStatGateway;

        for (size_t i = 0;
             i < net.v4dns.size() && i < std::size(msg.nic_dns_ip); ++i)
        {
            msg.nic_dns_ip[i] = std::byteswap(net.v4dns[i]);
        }

        if (net.v4Dhcp)
        {
            msg.dhcp_options = netDhcpEnabled;
            msg.dhcp_status = netDhcpStatIpAddr;
        }
    }

    net.hostName.copy(reinterpret_cast<char*>(msg.nic_name),
                      sizeof(msg.nic_name) - 1);
    net.domainName.copy(reinterpret_cast<char*>(msg.nic_domain_name),
                        sizeof(msg.nic_domain_name) - 1);

    return emitResponse(hdr, response, msg);
}

int SmifService::handleIPv6Config(const ChifPktHeader& hdr,
                                  std::span<uint8_t> response)
{
    const NetworkConfig net = query(bus_, netInterface_);

    Pkt8120 msg{};

    if (net.haveV4)
    {
        msg.v6options |= v6OptIpv4Enabled;
    }
    if (!net.v6addrs.empty() || net.v6AcceptRa || net.v6Dhcp)
    {
        msg.v6options |= v6OptIpv6Enabled;
    }
    if (net.v6Dhcp)
    {
        msg.dhcpv6options |= dhcpv6Stateful;
    }
    if (net.v6AcceptRa)
    {
        msg.dhcpv6options |= dhcpv6Stateless;
    }

    for (size_t i = 0; i < net.v6addrs.size() && i < std::size(msg.address);
         ++i)
    {
        const auto& e = net.v6addrs[i];
        std::ranges::copy(e.addr, msg.address[i].addr);
        msg.address[i].prefix = e.prefix;
        msg.address[i].source = static_cast<uint8_t>(e.origin);
    }

    for (size_t i = 0; i < net.v6dns.size() && i < std::size(msg.dns); ++i)
    {
        std::ranges::copy(net.v6dns[i], msg.dns[i].addr);
    }

    if (net.haveV6Gateway)
    {
        std::ranges::copy(net.v6gateway, msg.gateway.addr);
    }

    return emitResponse(hdr, response, msg);
}

} // namespace chif
