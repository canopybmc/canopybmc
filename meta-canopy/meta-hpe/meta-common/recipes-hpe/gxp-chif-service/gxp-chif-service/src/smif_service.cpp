// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#include "smif_service.hpp"

#include <phosphor-logging/lg2.hpp>

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <format>

namespace chif
{

SmifService::SmifService(EvStorage* evStorage,
                         std::unordered_map<uint8_t, int> segmentBusMap) :
    evStorage_(evStorage), segmentToBus_(std::move(segmentBusMap))
{}

// ---------------------------------------------------------------------------
// Response helpers
// ---------------------------------------------------------------------------

int SmifService::buildSimpleResponse(const ChifPktHeader& hdr,
                                     std::span<uint8_t> response,
                                     uint32_t errorCode)
{
    constexpr auto respSize =
        static_cast<uint16_t>(sizeof(ChifPktHeader) + sizeof(uint32_t));
    if (response.size() < respSize)
    {
        return -1;
    }
    initResponse(response, hdr, respSize);
    auto resp = responsePayload(response);
    std::memcpy(resp.data(), &errorCode, sizeof(errorCode));
    return respSize;
}

int SmifService::buildEvDataResponse(const ChifPktHeader& hdr,
                                     std::span<uint8_t> response,
                                     const EvEntry& ev)
{
    // Layout: [header 8][errorCode 4][name 32][dataLen 2][data N]
    // HPE pads zero-length EVs to 4 bytes on readback
    size_t dataSize = ev.data.empty() ? 4 : ev.data.size();
    size_t payloadSize = sizeof(uint32_t) + maxEvNameLen + sizeof(uint16_t) +
                         dataSize;
    auto respSize =
        static_cast<uint16_t>(sizeof(ChifPktHeader) + payloadSize);

    if (response.size() < respSize)
    {
        return buildSimpleResponse(hdr, response, 3);
    }

    std::fill_n(response.data(), respSize, uint8_t{0});
    initResponse(response, hdr, respSize);

    auto resp = responsePayload(response);
    // errorCode = 0 (already zeroed)

    // Name at offset 4 (response already zeroed, so null-padding is implicit)
    size_t nameLen = std::min(ev.name.size(), maxEvNameLen - 1);
    std::memcpy(resp.data() + sizeof(uint32_t), ev.name.c_str(), nameLen);

    // dataLen at offset 36 (use padded size for zero-length EVs)
    auto dataLen = static_cast<uint16_t>(dataSize);
    std::memcpy(resp.data() + sizeof(uint32_t) + maxEvNameLen, &dataLen,
                sizeof(dataLen));

    // data at offset 38 (zero-length EVs get 4 zero bytes from fill_n)
    if (!ev.data.empty())
    {
        std::memcpy(
            resp.data() + sizeof(uint32_t) + maxEvNameLen + sizeof(uint16_t),
            ev.data.data(), ev.data.size());
    }

    return respSize;
}

// ---------------------------------------------------------------------------
// EV command handlers
// ---------------------------------------------------------------------------

int SmifService::handleGetEvByIndex(const ChifPktHeader& hdr,
                                    std::span<const uint8_t> reqPayload,
                                    std::span<uint8_t> response)
{
    if (!evStorage_ || reqPayload.size() < sizeof(uint32_t))
    {
        return buildSimpleResponse(hdr, response, 1);
    }

    uint32_t index = 0;
    std::memcpy(&index, reqPayload.data(), sizeof(index));

    auto ev = evStorage_->getByIndex(index);
    if (!ev)
    {
        return buildSimpleResponse(hdr, response, 1);
    }
    return buildEvDataResponse(hdr, response, *ev);
}

int SmifService::handleSetDeleteEv(const ChifPktHeader& hdr,
                                   std::span<const uint8_t> reqPayload,
                                   std::span<uint8_t> response)
{
    if (!evStorage_ || reqPayload.size() < sizeof(uint32_t))
    {
        return buildSimpleResponse(hdr, response, 2);
    }

    uint8_t flags = reqPayload[0];

    // Priority: deleteAll > delete > set
    if (flags & evFlagDeleteAll)
    {
        bool ok = evStorage_->deleteAll();
        return buildSimpleResponse(hdr, response, ok ? 0 : 3);
    }

    if (flags & evFlagDelete)
    {
        if (reqPayload.size() < sizeof(uint32_t) + maxEvNameLen)
        {
            return buildSimpleResponse(hdr, response, 2);
        }
        std::string_view nameRegion(
            reinterpret_cast<const char*>(reqPayload.data()) +
                sizeof(uint32_t),
            maxEvNameLen);
        if (nameRegion.back() != '\0')
        {
            return buildSimpleResponse(hdr, response, 3);
        }
        bool ok = evStorage_->del(std::string(nameRegion.data()));
        return buildSimpleResponse(hdr, response, ok ? 0 : 1);
    }

    if (flags & evFlagSet)
    {
        constexpr size_t minSetPayload =
            sizeof(uint32_t) + maxEvNameLen + sizeof(uint16_t);
        if (reqPayload.size() < minSetPayload)
        {
            return buildSimpleResponse(hdr, response, 2);
        }

        char nameBuf[maxEvNameLen] = {};
        std::memcpy(nameBuf, reqPayload.data() + sizeof(uint32_t),
                    maxEvNameLen - 1);

        uint16_t dataLen = 0;
        std::memcpy(&dataLen,
                    reqPayload.data() + sizeof(uint32_t) + maxEvNameLen,
                    sizeof(dataLen));

        if (dataLen > maxEvDataSize)
        {
            return buildSimpleResponse(hdr, response, 3);
        }

        // HPE behavior: set with sz_ev=0 is treated as delete
        if (dataLen == 0)
        {
            evStorage_->del(nameBuf);
            return buildSimpleResponse(hdr, response, 0);
        }

        if (reqPayload.size() < minSetPayload + dataLen)
        {
            return buildSimpleResponse(hdr, response, 2);
        }

        auto data = reqPayload.subspan(minSetPayload, dataLen);
        bool ok = evStorage_->set(nameBuf, data);
        return buildSimpleResponse(hdr, response, ok ? 0 : 3);
    }

    return buildSimpleResponse(hdr, response, 2);
}

int SmifService::handleGetEvByName(const ChifPktHeader& hdr,
                                   std::span<const uint8_t> reqPayload,
                                   std::span<uint8_t> response)
{
    if (reqPayload.size() < maxEvNameLen)
    {
        return buildSimpleResponse(hdr, response, 2);
    }
    if (!evStorage_)
    {
        return buildSimpleResponse(hdr, response, 1);
    }
    if (reqPayload[maxEvNameLen - 1] != 0x00)
    {
        return buildSimpleResponse(hdr, response, 3);
    }

    char nameBuf[maxEvNameLen] = {};
    std::memcpy(nameBuf, reqPayload.data(), maxEvNameLen - 1);

    auto ev = evStorage_->getByName(nameBuf);
    if (!ev)
    {
        return buildSimpleResponse(hdr, response, 1);
    }
    return buildEvDataResponse(hdr, response, *ev);
}

int SmifService::handleEvStats(const ChifPktHeader& hdr,
                               std::span<uint8_t> response)
{
    // HPE layout: [errorCode u32][rem_sz u32][present_evs u16][max_sz u16]
    constexpr size_t payloadSize =
        sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint16_t) +
        sizeof(uint16_t); // 12 bytes
    constexpr auto respSize =
        static_cast<uint16_t>(sizeof(ChifPktHeader) + payloadSize);

    if (response.size() < respSize)
    {
        return -1;
    }

    std::fill_n(response.data(), respSize, uint8_t{0});
    initResponse(response, hdr, respSize);

    auto resp = responsePayload(response);
    // errorCode = 0 (already zeroed)

    uint32_t remaining = evStorage_ ? static_cast<uint32_t>(
                                          evStorage_->remainingSize())
                                    : 0;
    auto cnt = static_cast<uint16_t>(evStorage_ ? evStorage_->count() : 0);
    auto maxSzKb = static_cast<uint16_t>(EvStorage::maxSize() / 1024);

    std::memcpy(resp.data() + 4, &remaining, sizeof(remaining));
    std::memcpy(resp.data() + 8, &cnt, sizeof(cnt));
    std::memcpy(resp.data() + 10, &maxSzKb, sizeof(maxSzKb));

    return respSize;
}

int SmifService::handleEvState(const ChifPktHeader& hdr,
                               std::span<uint8_t> response)
{
    // HPE layout: [errorCode u32][vsp_connection_status u32]
    constexpr size_t payloadSize = 2 * sizeof(uint32_t); // 8 bytes
    constexpr auto respSize =
        static_cast<uint16_t>(sizeof(ChifPktHeader) + payloadSize);

    if (response.size() < respSize)
    {
        return -1;
    }

    std::fill_n(response.data(), respSize, uint8_t{0});
    initResponse(response, hdr, respSize);

    auto resp = responsePayload(response);
    // errorCode = 0 (already zeroed)
    uint32_t connected = 1; // VSP connected
    std::memcpy(resp.data() + 4, &connected, sizeof(connected));

    return respSize;
}

int SmifService::handleGetEvAuthStatus(const ChifPktHeader& hdr,
                                       std::span<uint8_t> response)
{
    // Return "all authenticated" — matches HPE's OpenBMC behavior.
    // HPE returns hardcoded values without performing real verification.
    // pkt_812d is 124 bytes of payload.
    constexpr size_t authPayloadSize = 124;
    constexpr auto respSize =
        static_cast<uint16_t>(sizeof(ChifPktHeader) + authPayloadSize);

    if (response.size() < respSize)
    {
        return -1;
    }

    std::fill_n(response.data(), respSize, uint8_t{0});
    initResponse(response, hdr, respSize);

    auto resp = responsePayload(response);
    // ErrorCode = 0 (already zeroed)
    uint32_t version = 1;
    std::memcpy(resp.data() + 4, &version, sizeof(version)); // AuthVersion
    resp[8] = 0x01;  // ImageAuthBitField: single side A active
    resp[9] = 0x01;  // SideA: authenticated
    resp[10] = 0x01; // SideB: authenticated
    // RemediationAction at [11] = 0 (none)
    // ValidatingAgent at [58..59] = 0 (BMC)
    // All other fields (signatures, GUIDs) left zeroed

    return respSize;
}

// ---------------------------------------------------------------------------
// I2C proxy handler (0x0072)
// ---------------------------------------------------------------------------

// pkt_0072 payload offsets
static constexpr size_t i2cOffAddress = 12;
static constexpr size_t i2cOffSegment = 14;
static constexpr size_t i2cOffWriteLen = 15;
static constexpr size_t i2cOffReadLen = 16;
static constexpr size_t i2cOffData = 17;
static constexpr size_t i2cMinPayload = 17;

// pkt_8072 response layout (matches HPE's packed struct)
struct I2cResponse
{
    uint32_t errcode;
    uint8_t reserved1[8];
    uint16_t address;
    uint8_t engine;
    uint8_t reserved2;
    uint8_t readLen;
    uint8_t data[32];
} __attribute__((packed));

static_assert(sizeof(I2cResponse) == 49, "I2cResponse must be 49 bytes");

static constexpr auto i2cRespSize =
    static_cast<uint16_t>(sizeof(ChifPktHeader) + sizeof(I2cResponse));
static constexpr size_t i2cRespOffError = offsetof(I2cResponse, errcode);
static constexpr size_t i2cRespOffAddress = offsetof(I2cResponse, address);
static constexpr size_t i2cRespOffSegment = offsetof(I2cResponse, engine);
static constexpr size_t i2cRespOffReadLen = offsetof(I2cResponse, readLen);
static constexpr size_t i2cRespOffData = offsetof(I2cResponse, data);

// I2C error codes (100-series, from HPE i2c_return_codes.hpp)
static constexpr uint32_t i2cGeneralError = 101;
static constexpr uint32_t i2cBusTimeout = 105;
static constexpr uint32_t i2cSegmentNotFound = 108;
static constexpr uint32_t i2cAddressNack = 116;

static uint32_t i2cErrorFromErrno(int err)
{
    switch (err)
    {
        case ENXIO:
            return i2cAddressNack;
        case ETIMEDOUT:
            return i2cBusTimeout;
        default:
            return i2cGeneralError;
    }
}

int SmifService::handleI2cProxy(const ChifPktHeader& hdr,
                                std::span<const uint8_t> reqPayload,
                                std::span<uint8_t> response)
{
    if (response.size() < i2cRespSize)
    {
        return -1;
    }

    std::fill_n(response.data(), i2cRespSize, uint8_t{0});
    initResponse(response, hdr, i2cRespSize);
    auto resp = responsePayload(response);

    if (reqPayload.size() < i2cMinPayload)
    {
        uint32_t err = i2cGeneralError;
        std::memcpy(resp.data() + i2cRespOffError, &err, sizeof(err));
        return i2cRespSize;
    }

    uint16_t address = 0;
    std::memcpy(&address, reqPayload.data() + i2cOffAddress, sizeof(address));
    uint8_t segment = reqPayload[i2cOffSegment];
    uint8_t writeLen = reqPayload[i2cOffWriteLen];
    uint8_t readLen = reqPayload[i2cOffReadLen];

    // Echo address and segment in response
    std::memcpy(resp.data() + i2cRespOffAddress, &address, sizeof(address));
    resp[i2cRespOffSegment] = segment;

    if (writeLen == 0 && readLen == 0)
    {
        return i2cRespSize;
    }

    if (writeLen > 32 || readLen > 32)
    {
        uint32_t err = i2cGeneralError;
        std::memcpy(resp.data() + i2cRespOffError, &err, sizeof(err));
        return i2cRespSize;
    }

    auto it = segmentToBus_.find(segment);
    if (it == segmentToBus_.end())
    {
        uint32_t err = i2cSegmentNotFound;
        std::memcpy(resp.data() + i2cRespOffError, &err, sizeof(err));
        return i2cRespSize;
    }

    int busNum = it->second;
    uint8_t i2cAddr = static_cast<uint8_t>(address >> 1);

    auto devPath = std::format("/dev/i2c-{}", busNum);

    int fd = open(devPath.c_str(), O_RDWR);
    if (fd < 0)
    {
        lg2::warning("I2C proxy: failed to open {DEV}: {ERR}",
                     "DEV", devPath, "ERR", strerror(errno));
        uint32_t err = i2cGeneralError;
        std::memcpy(resp.data() + i2cRespOffError, &err, sizeof(err));
        return i2cRespSize;
    }

    struct i2c_msg msgs[2];
    int numMsgs = 0;
    uint8_t writeBuf[32] = {};
    uint8_t readBuf[32] = {};

    if (writeLen > 0)
    {
        size_t available = reqPayload.size() - i2cOffData;
        if (static_cast<size_t>(writeLen) > available)
        {
            uint32_t err = i2cGeneralError;
            std::memcpy(resp.data() + i2cRespOffError, &err, sizeof(err));
            close(fd);
            return i2cRespSize;
        }
        std::memcpy(writeBuf, reqPayload.data() + i2cOffData, writeLen);

        msgs[numMsgs].addr = i2cAddr;
        msgs[numMsgs].flags = 0;
        msgs[numMsgs].len = writeLen;
        msgs[numMsgs].buf = writeBuf;
        numMsgs++;
    }

    if (readLen > 0)
    {
        msgs[numMsgs].addr = i2cAddr;
        msgs[numMsgs].flags = I2C_M_RD;
        msgs[numMsgs].len = readLen;
        msgs[numMsgs].buf = readBuf;
        numMsgs++;
    }

    struct i2c_rdwr_ioctl_data transfer{};
    transfer.msgs = msgs;
    transfer.nmsgs = static_cast<uint32_t>(numMsgs);

    int rc = ioctl(fd, I2C_RDWR, &transfer);
    close(fd);

    if (rc < 0)
    {
        int savedErrno = errno;
        lg2::warning("I2C proxy: ioctl failed seg=0x{SEG} addr=0x{ADDR} "
                     "bus={BUS}: {ERR}",
                     "SEG", lg2::hex, segment, "ADDR", lg2::hex, i2cAddr,
                     "BUS", busNum, "ERR", strerror(savedErrno));
        uint32_t err = i2cErrorFromErrno(savedErrno);
        std::memcpy(resp.data() + i2cRespOffError, &err, sizeof(err));
        return i2cRespSize;
    }

    resp[i2cRespOffReadLen] = readLen;
    if (readLen > 0)
    {
        std::memcpy(resp.data() + i2cRespOffData, readBuf, readLen);
    }

    return i2cRespSize;
}

// ---------------------------------------------------------------------------
// SmifService::handle — main dispatch
// ---------------------------------------------------------------------------
int SmifService::handle(std::span<const uint8_t> request,
                        std::span<uint8_t> response)
{
    if (request.size() < sizeof(ChifPktHeader))
    {
        return -1;
    }

    constexpr auto respSize =
        static_cast<uint16_t>(sizeof(ChifPktHeader) + sizeof(uint32_t));

    if (response.size() < respSize)
    {
        return -1;
    }

    auto hdr = parseHeader(request);
    auto reqPayload = payload(request);

    switch (hdr.command)
    {
        // ---- EV storage (real implementation) ----
        case smifCmdGetEvByIndex:
            return handleGetEvByIndex(hdr, reqPayload, response);
        case smifCmdSetDeleteEv:
            return handleSetDeleteEv(hdr, reqPayload, response);
        case smifCmdGetEvByName:
            return handleGetEvByName(hdr, reqPayload, response);
        case smifCmdEvStats:
            return handleEvStats(hdr, response);
        case smifCmdEvState:
            return handleEvState(hdr, response);

        // ---- BIOS image auth status (Secure Start) ----
        case smifCmdGetEvAuthStatus:
            return handleGetEvAuthStatus(hdr, response);

        // ---- I2C proxy ----
        case smifCmdI2cTransaction:
            return handleI2cProxy(hdr, reqPayload, response);

        // ---- All other commands: stub with ErrorCode=0 ----
        default:
            return buildSimpleResponse(hdr, response, 0);
    }
}

} // namespace chif
