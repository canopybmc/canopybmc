// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH

#include "smif_service.hpp"
#include "platform_info.hpp"
#include "unique_fd.hpp"

#include <phosphor-logging/lg2.hpp>

#include <cstring>
#include <fstream>
#include <sstream>

#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>

namespace chif
{

int SmifService::handleSecurityStateGet(const ChifPktHeader& reqHdr,
                                        std::span<uint8_t> response)
{
    // pkt_8139: rcode(4) + security_state(1) + reserved(3) = 8 bytes payload
    constexpr uint16_t payloadSize = 8;
    constexpr uint16_t respSize =
        static_cast<uint16_t>(sizeof(ChifPktHeader) + payloadSize);

    if (response.size() < respSize)
    {
        return -1;
    }

    initResponse(response, reqHdr, respSize);
    auto resp = responsePayload(response);
    std::memset(resp.data(), 0, payloadSize);

    uint32_t rcode = 0; // success
    std::memcpy(resp.data(), &rcode, sizeof(rcode));
    resp[4] = 3; // PRODUCTION mode

    return respSize;
}

int SmifService::handleSecurityStateSet(const ChifPktHeader& reqHdr,
                                        std::span<const uint8_t> reqPayload,
                                        std::span<uint8_t> response)
{
    // BIOS sends security state value — accept and log it.
    uint32_t secState = 0;
    if (reqPayload.size() >= 4)
    {
        std::memcpy(&secState, reqPayload.data(), sizeof(secState));
    }
    lg2::info("SMIF 0x0158: BIOS set security state = {STATE}", "STATE",
              secState);

    return buildSimpleResponse(reqHdr, response, 0);
}

int SmifService::handlePostState(const ChifPktHeader& reqHdr,
                                 std::span<const uint8_t> reqPayload,
                                 std::span<uint8_t> response)
{
    // pkt_0143: post_state(1) + reserved(3)
    uint8_t postState = 0;
    if (reqPayload.size() >= 1)
    {
        postState = reqPayload[0];
    }
    lg2::info("SMIF 0x0143: POST state = 0x{STATE:02x}", "STATE", postState);

    return buildSimpleResponse(reqHdr, response, 0);
}

int SmifService::handleLicenseKey(const ChifPktHeader& reqHdr,
                                  std::span<uint8_t> response)
{
    // pkt_806e: ErrCode(4) + flags(4) + mask(4) + installable(4) +
    //           status(4) + reserved(8) + key(100) = 128 bytes payload
    constexpr uint16_t payloadSize = 128;
    constexpr uint16_t respSize =
        static_cast<uint16_t>(sizeof(ChifPktHeader) + payloadSize);

    if (response.size() < respSize)
    {
        return -1;
    }

    initResponse(response, reqHdr, respSize);
    auto resp = responsePayload(response);
    std::memset(resp.data(), 0, payloadSize);

    // ErrCode=1 means license IS installed (HPE convention)
    uint32_t errCode = 1;
    std::memcpy(resp.data(), &errCode, sizeof(errCode));

    // flags=0 (no demo)
    // mask=0x77 (Advanced+Light+Select+StdBlade+Essentials+ScaleOut)
    uint32_t mask = 0x77;
    std::memcpy(resp.data() + 8, &mask, sizeof(mask));

    // installable=0x21 (Advanced+ScaleOut)
    uint32_t installable = 0x21;
    std::memcpy(resp.data() + 12, &installable, sizeof(installable));

    // status=1
    uint32_t status = 1;
    std::memcpy(resp.data() + 16, &status, sizeof(status));

    // key[0..24] = activation key, key[26..] = license type
    const char* activationKey = "CANOPY-OPENBMC-K";
    const char* licenseType = "OpenBMC";
    std::memcpy(resp.data() + 28, activationKey, strlen(activationKey));
    resp[28 + 25] = 0; // null separator
    std::memcpy(resp.data() + 28 + 26, licenseType, strlen(licenseType));

    return respSize;
}

int SmifService::handleFieldAccess(const ChifPktHeader& reqHdr,
                                   std::span<const uint8_t> reqPayload,
                                   std::span<uint8_t> response)
{
    // pkt_0153/8153: rcode(4) + op(4) + sz(4) + res(4) + buf(256) = 268 bytes
    constexpr uint16_t payloadSize = 268;
    constexpr uint16_t respSize =
        static_cast<uint16_t>(sizeof(ChifPktHeader) + payloadSize);

    if (response.size() < respSize)
    {
        return -1;
    }

    initResponse(response, reqHdr, respSize);
    auto resp = responsePayload(response);
    std::memset(resp.data(), 0, payloadSize);

    // Parse operation code
    uint32_t op = 0;
    if (reqPayload.size() >= 8)
    {
        std::memcpy(&op, reqPayload.data() + 4, sizeof(op));
    }

    uint32_t rcode = 0;
    uint32_t sz = 0;

    switch (op)
    {
        case 1: // OP153_RD_SN — Read serial number
        {
            sz = static_cast<uint32_t>(boardSerial_.size());
            if (sz > 256)
                sz = 256;
            std::memcpy(resp.data() + 16, boardSerial_.data(), sz);
            lg2::info("SMIF 0x0153: read S/N = '{SN}'", "SN", boardSerial_);
            break;
        }
        case 3: // OP153_RD_PRODID — Read product ID
        {
            sz = static_cast<uint32_t>(productId_.size());
            if (sz > 256)
                sz = 256;
            std::memcpy(resp.data() + 16, productId_.data(), sz);
            lg2::info("SMIF 0x0153: read ProdID = '{PID}'", "PID",
                      productId_);
            break;
        }
        case 5: // OP153_RD_PCANUM
        case 7: // OP153_RD_PCAPART
        case 9: // OP153_RD_CR_BRAND
        case 10: // OP153_RD_CR_PRODID
        {
            // Return empty for unimplemented read ops
            rcode = 0;
            sz = 0;
            break;
        }
        case 2:  // OP153_WR_SN
        case 4:  // OP153_WR_PRODID
        case 6:  // OP153_WR_PCANUM
        case 8:  // OP153_WR_PCAPART
        case 11: // OP153_WR_CR_PRODID
        {
            // Accept writes silently
            rcode = 0;
            lg2::info("SMIF 0x0153: write op={OP} accepted", "OP", op);
            break;
        }
        default:
            rcode = 2; // not implemented
            break;
    }

    std::memcpy(resp.data(), &rcode, sizeof(rcode));
    std::memcpy(resp.data() + 4, &op, sizeof(op));
    std::memcpy(resp.data() + 8, &sz, sizeof(sz));

    return respSize;
}

int SmifService::handleBootProgress(const ChifPktHeader& reqHdr,
                                    std::span<const uint8_t> reqPayload,
                                    std::span<uint8_t> response)
{
    // pkt_0209: progressCode(2) + reserved(2) + bootTime(4)
    uint16_t progressCode = 0;
    uint32_t bootTime = 0;
    if (reqPayload.size() >= 2)
    {
        std::memcpy(&progressCode, reqPayload.data(), sizeof(progressCode));
    }
    if (reqPayload.size() >= 8)
    {
        std::memcpy(&bootTime, reqPayload.data() + 4, sizeof(bootTime));
    }
    lg2::info("SMIF 0x0209: boot progress code=0x{CODE:04x} time={TIME}s",
              "CODE", progressCode, "TIME", bootTime);

    return buildSimpleResponse(reqHdr, response, 0);
}

int SmifService::handleBiosSync(const ChifPktHeader& reqHdr,
                                std::span<const uint8_t> reqPayload,
                                std::span<uint8_t> response)
{
    // pkt_0136: flags(4) — 0x01=EV sync set, 0x00=EV sync clear
    uint32_t flags = 0;
    if (reqPayload.size() >= 4)
    {
        std::memcpy(&flags, reqPayload.data(), sizeof(flags));
    }
    lg2::info("SMIF 0x0136: BIOS sync flags=0x{FLAGS:08x}", "FLAGS", flags);

    return buildSimpleResponse(reqHdr, response, 0);
}

int SmifService::handleGpioCpld(const ChifPktHeader& reqHdr,
                                std::span<const uint8_t> reqPayload,
                                std::span<uint8_t> response)
{
    // pkt_0088/8088: op(4) + index(4) + resv1(4) + status(4) + resv2(3) + val(1)
    // = 20 bytes payload. Same struct for request and response.
    constexpr uint16_t payloadSize = 20;
    constexpr uint16_t respSize =
        static_cast<uint16_t>(sizeof(ChifPktHeader) + payloadSize);

    if (response.size() < respSize)
    {
        return -1;
    }

    initResponse(response, reqHdr, respSize);
    auto resp = responsePayload(response);
    std::memset(resp.data(), 0, payloadSize);

    // Parse request
    uint32_t op = 0, index = 0;
    if (reqPayload.size() >= 4)
    {
        std::memcpy(&op, reqPayload.data(), sizeof(op));
    }
    if (reqPayload.size() >= 8)
    {
        std::memcpy(&index, reqPayload.data() + 4, sizeof(index));
    }

    // Echo op and index
    std::memcpy(resp.data(), &op, sizeof(op));
    std::memcpy(resp.data() + 4, &index, sizeof(index));

    uint32_t status = 0; // SMIF_SUCCESS
    uint8_t val = 0;

    switch (op)
    {
        case 3: // SMIF_GET_CPLD — Read Xregister (used for SERVER_ID)
        {
            // Read server_id from sysfs using std::ifstream (RAII)
            std::ifstream in("/sys/class/soc/xreg/server_id");
            unsigned int serverId = 0;
            if (in >> std::hex >> serverId)
            {
                if (index == 1)
                {
                    val = static_cast<uint8_t>(serverId & 0xFF);
                }
                else if (index == 2)
                {
                    val = static_cast<uint8_t>((serverId >> 8) & 0xFF);
                }
                else
                {
                    // Return 0 for unknown indices (including Platform ID).
                    // Returning SMIF_BAD_INDEX causes "Device Error" in BIOS.
                    val = 0;
                    status = 0;
                }
            }
            else
            {
                status = 0;
                val = 0;
            }
            break;
        }
        case 1: // SMIF_GET_MEMID
        case 2: // SMIF_GET_GPI
        case 6: // SMIF_GET_GPIEN
        case 7: // SMIF_GET_GPIST
        case 10: // SMIF_GET_GPO
        {
            // Read operations — return 0
            val = 0;
            status = 0;
            break;
        }
        case 4: // SMIF_PUT_GPO
        case 5: // SMIF_PUT_CPLD
        case 8: // SMIF_PUT_GPIEN
        case 9: // SMIF_PUT_GPIST
        {
            // Write operations — accept silently
            status = 0;
            break;
        }
        default:
            status = 1; // SMIF_BAD_OP
            break;
    }

    // Write status at offset 12 and val at offset 19
    std::memcpy(resp.data() + 12, &status, sizeof(status));
    resp[19] = val;

    return respSize;
}

int SmifService::handlePowerRegulator(const ChifPktHeader& reqHdr,
                                      std::span<const uint8_t> /* reqPayload */,
                                      std::span<uint8_t> response)
{
    // Accept power regulator config from BIOS. Log and ACK.
    lg2::info("SMIF 0x0176: power regulator config received");
    return buildSimpleResponse(reqHdr, response, 0);
}

// Helper: read network interface info via ioctl
static bool getIfaceInfo(const char* ifName, uint32_t& ipAddr,
                         uint32_t& netmask, uint32_t& gateway,
                         uint8_t* macAddr)
{
    UniqueFd fd(socket(AF_INET, SOCK_DGRAM, 0));
    if (fd.fd < 0)
    {
        return false;
    }

    struct ifreq ifr{};
    strncpy(ifr.ifr_name, ifName, IFNAMSIZ - 1);

    // IP address
    if (ioctl(fd, SIOCGIFADDR, &ifr) == 0)
    {
        auto* sin = reinterpret_cast<struct sockaddr_in*>(&ifr.ifr_addr);
        ipAddr = sin->sin_addr.s_addr;
    }

    // Netmask
    if (ioctl(fd, SIOCGIFNETMASK, &ifr) == 0)
    {
        auto* sin = reinterpret_cast<struct sockaddr_in*>(&ifr.ifr_netmask);
        netmask = sin->sin_addr.s_addr;
    }

    // MAC address
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0)
    {
        std::memcpy(macAddr, ifr.ifr_hwaddr.sa_data, 6);
    }

    // Gateway: read from /proc/net/route
    gateway = 0;
    {
        std::ifstream routeFile("/proc/net/route");
        std::string line;
        while (std::getline(routeFile, line))
        {
            std::istringstream iss(line);
            std::string iface;
            uint32_t dest = 0;
            uint32_t gw = 0;
            if ((iss >> iface >> std::hex >> dest >> gw) &&
                iface == ifName && dest == 0)
            {
                gateway = gw;
                break;
            }
        }
    }

    return ipAddr != 0;
}

int SmifService::fillNetworkResponse(const ChifPktHeader& reqHdr,
                                     std::span<uint8_t> response)
{
    constexpr uint16_t payloadSize = 298;
    constexpr uint16_t respSize =
        static_cast<uint16_t>(sizeof(ChifPktHeader) + payloadSize);

    if (response.size() < respSize)
    {
        return -1;
    }

    initResponse(response, reqHdr, respSize);
    auto resp = responsePayload(response);
    std::memset(resp.data(), 0, payloadSize);

    uint32_t ipAddr = 0, netmask = 0, gateway = 0;
    uint8_t mac[6] = {};

    if (getIfaceInfo("eth0", ipAddr, netmask, gateway, mac))
    {
        uint32_t errCode = 0;
        std::memcpy(resp.data(), &errCode, sizeof(errCode));

        // IP address at offset 12 (host-to-network byte order for BIOS)
        uint32_t netIp = ntohl(ipAddr);
        std::memcpy(resp.data() + 12, &netIp, sizeof(netIp));

        // Subnet mask at offset 16
        uint32_t netMask = ntohl(netmask);
        std::memcpy(resp.data() + 16, &netMask, sizeof(netMask));

        // MAC at offset 20
        std::memcpy(resp.data() + 20, mac, 6);

        // Gateway at offset 26
        uint32_t netGw = ntohl(gateway);
        std::memcpy(resp.data() + 26, &netGw, sizeof(netGw));

        // Hostname at offset 198
        char hostname[64] = {};
        gethostname(hostname, sizeof(hostname) - 1);
        std::memcpy(resp.data() + 198, hostname, strlen(hostname));

        // Log with inet_ntop (thread-safe, no static buffer)
        char ipStr[INET_ADDRSTRLEN] = {};
        char maskStr[INET_ADDRSTRLEN] = {};
        char gwStr[INET_ADDRSTRLEN] = {};
        inet_ntop(AF_INET, &ipAddr, ipStr, sizeof(ipStr));
        inet_ntop(AF_INET, &netmask, maskStr, sizeof(maskStr));
        inet_ntop(AF_INET, &gateway, gwStr, sizeof(gwStr));
        lg2::info("SMIF 0x{CMD:04x}: IPv4 — IP={IP} mask={MASK} gw={GW}",
                  "CMD", reqHdr.command, "IP", ipStr, "MASK", maskStr, "GW",
                  gwStr);
    }
    else
    {
        // No network available
        uint32_t errCode = 1;
        std::memcpy(resp.data(), &errCode, sizeof(errCode));
    }

    return respSize;
}

int SmifService::handleIPv4Config(const ChifPktHeader& reqHdr,
                                  std::span<uint8_t> response)
{
    return fillNetworkResponse(reqHdr, response);
}

int SmifService::handleNicConfig(const ChifPktHeader& reqHdr,
                                 std::span<uint8_t> response)
{
    return fillNetworkResponse(reqHdr, response);
}

int SmifService::handlePciDeviceInfo(const ChifPktHeader& reqHdr,
                                     std::span<const uint8_t> reqPayload,
                                     std::span<uint8_t> response)
{
    // BIOS sends PCI topology data — accept and log device count.
    // pkt_0035 payload: revision(1) + pad(1) + pciDevCnt(2) + entries[]
    // Each entry is 14 bytes (BDF, VID, DID, SubVen, SubDev, ParentBDF,
    // DevLocFlags, DevLocNum, Class, SubClass).
    uint16_t devCount = 0;
    if (reqPayload.size() >= 4)
    {
        std::memcpy(&devCount, reqPayload.data() + 2, sizeof(devCount));
    }

    lg2::info("SMIF 0x0035: PCI device info received — {CNT} devices",
              "CNT", devCount);

    // For now just ACK. Phase 2 would parse entries and create
    // PCIe.Device D-Bus objects for Redfish.
    return buildSimpleResponse(reqHdr, response, 0);
}

int SmifService::handleIPv6Config(const ChifPktHeader& reqHdr,
                                  std::span<uint8_t> response)
{
    // pkt_8120: errcode(4) + IPv6Option(4) + ... (~568 bytes)
    // Return "IPv6 not configured" which is safe and matches baseline.
    constexpr uint16_t payloadSize = 8;
    constexpr uint16_t respSize =
        static_cast<uint16_t>(sizeof(ChifPktHeader) + payloadSize);

    if (response.size() < respSize)
    {
        return -1;
    }

    initResponse(response, reqHdr, respSize);
    auto resp = responsePayload(response);
    std::memset(resp.data(), 0, payloadSize);

    // ErrorCode=1 (not configured), IPv6Option=0 (disabled)
    uint32_t errCode = 1;
    std::memcpy(resp.data(), &errCode, sizeof(errCode));

    return respSize;
}

} // namespace chif
