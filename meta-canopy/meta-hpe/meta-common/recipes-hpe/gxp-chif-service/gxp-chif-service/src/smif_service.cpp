// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#include "smif_service.hpp"

#include <phosphor-logging/lg2.hpp>

#include <algorithm>
#include <cstring>

namespace chif
{

SmifService::SmifService(EvStorage* evStorage) : evStorage_(evStorage) {}

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

        // ---- EV auth status (always "not implemented") ----
        case smifCmdGetEvAuthStatus:
            return buildSimpleResponse(hdr, response, 1);

        // ---- All other commands: stub with ErrorCode=0 ----
        default:
            return buildSimpleResponse(hdr, response, 0);
    }
}

} // namespace chif
