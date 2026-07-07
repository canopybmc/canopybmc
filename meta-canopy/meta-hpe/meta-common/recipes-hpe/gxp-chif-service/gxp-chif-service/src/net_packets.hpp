// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#pragma once

#include <cstdint>

namespace chif
{

// Magic value for v3 config layout
inline constexpr uint32_t netCfgMagic = 0x439cd202;
inline constexpr uint32_t netCfgVersion = 3;
inline constexpr uint32_t netSetIpv4Enabled = 0x80;
inline constexpr uint32_t netSetIpv6Enabled = 0x100;
inline constexpr uint32_t netStatIpAddr = 0x1;
inline constexpr uint32_t netStatGateway = 0x2;
inline constexpr uint32_t netStatDns = 0x4;
inline constexpr uint32_t netDhcpEnabled = 0x1;
inline constexpr uint32_t netDhcpStatIpAddr = 0x1;

struct IodRoute
{
    uint32_t dest;
    uint32_t gate;
    uint32_t mask;
} __attribute__((packed));

struct IodIntf
{
    uint32_t settings;
    uint32_t status;
    uint32_t dhcp_options;
    uint32_t dhcp_status;
    uint32_t ipaddr;
    uint32_t ip_mask;
    uint32_t gateway_ip;
    uint32_t wins_ip[2];
    uint32_t dns_ip[3];
    uint32_t ntp_ip[2];
    IodRoute route[3];
    uint16_t vlan_id;
    uint16_t QoS;
    char host_name[63];
    char domain_name[193];
    uint8_t dhcp_params[64];
    uint8_t dhcp_clientid_len;
    uint8_t dhcp_clientid_value[16];
    uint8_t reserved[75];
    uint32_t kernel;
} __attribute__((packed));

static_assert(sizeof(IodIntf) == 512, "IodIntf must be 512 bytes");

struct IodCfg
{
    uint32_t magic;
    uint32_t version;
    uint32_t checksum;
    uint32_t sequence;
    uint32_t settings;
    uint8_t physel;
    uint8_t sideband_sel;
    uint8_t channel;
    uint8_t package;
    uint8_t hidden_port;
    uint8_t reserved[999];
    IodIntf iface[2];
} __attribute__((packed));

static_assert(sizeof(IodCfg) == 2048, "IodCfg must be 2048 bytes");

struct Pkt8006
{
    uint32_t errorCode;
    IodCfg cfg;
} __attribute__((packed));

struct Pkt8063
{
    uint32_t errorCode;
    uint32_t nic_settings;
    uint32_t nic_status;
    uint32_t nic_ipaddr;
    uint32_t nic_ip_mask;
    uint8_t nic_mac_addr[6];
    uint32_t nic_gateway_ip;
    uint32_t nic_wins_ip[2];
    uint32_t nic_dns_ip[3];
    uint32_t dhcp_server_ip_address;
    uint32_t dhcp_options;
    uint32_t dhcp_status;
    uint32_t front_nic_ipaddr;
    uint32_t front_nic_ip_mask;
    uint32_t front_settings;
    struct
    {
        uint32_t dest;
        uint32_t gate;
    } route[3];
    uint8_t nic_name[50];
    uint8_t nic_domain_name[128];
} __attribute__((packed));

inline constexpr uint32_t nicSettingsDefault = 0x3;

struct V6Addr
{
    uint8_t addr[16];
    uint8_t prefix;
    uint8_t source;
    uint8_t state;
    uint8_t reserved;
} __attribute__((packed));

static_assert(sizeof(V6Addr) == 20, "V6Addr must be 20 bytes");

struct Pkt8120
{
    uint32_t errorCode;
    uint32_t v6options;
    uint32_t dhcpv6options;
    V6Addr address[12];
    uint32_t reserved1[5];
    V6Addr dns[3];
    V6Addr gateway;
    V6Addr dest1;
    V6Addr gate1;
    V6Addr dest2;
    V6Addr gate2;
    V6Addr dest3;
    V6Addr gate3;
    uint8_t post_disp_cfg;
    uint8_t reserved2[31];
} __attribute__((packed));

static_assert(sizeof(Pkt8120) == 504, "Pkt8120 must be 504 bytes");

// v6options bits (HPE: "1 IPv6 enabled, 2 IPv4 enabled")
inline constexpr uint32_t v6OptIpv6Enabled = 0x1;
inline constexpr uint32_t v6OptIpv4Enabled = 0x2;

inline constexpr uint32_t dhcpv6Stateful = 0x1;
inline constexpr uint32_t dhcpv6Stateless = 0x2;

} // namespace chif
