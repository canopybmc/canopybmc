// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH

#include "smif_service.hpp"

#include <phosphor-logging/lg2.hpp>

#include <cstring>

namespace chif
{

int SmifService::handleGetEvByIndex(const ChifPktHeader& reqHdr,
                                    std::span<const uint8_t> reqPayload,
                                    std::span<uint8_t> response)
{
    uint32_t idx = 0;
    if (reqPayload.size() >= sizeof(uint32_t))
    {
        std::memcpy(&idx, reqPayload.data(), sizeof(idx));
    }

    auto ev = evStorage_.getByIndex(idx);
    if (!ev)
    {
        // Return ErrorCode=1 (not found). The BIOS HandleEvChifErrorResponse
        // function catches this and enters the error recovery path, which
        // skips EV processing and continues POST. Returning ErrorCode=0
        // causes the BIOS to enter HpSyncShadowedEvs which hangs.
        return buildSimpleResponse(reqHdr, response, 1);
    }

    return buildEvDataResponse(reqHdr, response, *ev);
}

int SmifService::handleSetDeleteEv(const ChifPktHeader& reqHdr,
                                   std::span<const uint8_t> reqPayload,
                                   std::span<uint8_t> response)
{
    // Minimum payload: flags(1) + reserved(3) = 4 bytes
    if (reqPayload.size() < 4)
    {
        return buildSimpleResponse(reqHdr, response, 2);
    }

    uint8_t flags = reqPayload[0];

    if (flags & evFlagDeleteAll)
    {
        bool ok = evStorage_.deleteAll();
        lg2::info("SMIF: EV delete all — {RESULT}", "RESULT",
                  ok ? "success" : "failed");
        return buildSimpleResponse(reqHdr, response, ok ? 0 : 3);
    }

    if (flags & evFlagDelete)
    {
        // Need: flags(4) + name(32) = 36 bytes
        if (reqPayload.size() < 36)
        {
            return buildSimpleResponse(reqHdr, response, 2);
        }

        char nameBuf[maxEvNameLen] = {};
        std::memcpy(nameBuf, reqPayload.data() + 4, maxEvNameLen);
        std::string name(nameBuf, strnlen(nameBuf, sizeof(nameBuf)));

        bool ok = evStorage_.del(name);
        lg2::info("SMIF: EV delete '{NAME}' — {RESULT}", "NAME", name,
                  "RESULT", ok ? "success" : "not found");
        return buildSimpleResponse(reqHdr, response, ok ? 0 : 1);
    }

    if (flags & evFlagSet)
    {
        // Need: flags(4) + name(32) + sz_ev(2) = 38 bytes minimum
        if (reqPayload.size() < 38)
        {
            return buildSimpleResponse(reqHdr, response, 2);
        }

        char nameBuf[maxEvNameLen] = {};
        std::memcpy(nameBuf, reqPayload.data() + 4, maxEvNameLen);
        std::string name(nameBuf, strnlen(nameBuf, sizeof(nameBuf)));

        uint16_t dataLen = 0;
        std::memcpy(&dataLen, reqPayload.data() + 36, sizeof(dataLen));

        if (reqPayload.size() < static_cast<size_t>(38) + dataLen)
        {
            return buildSimpleResponse(reqHdr, response, 2);
        }

        auto data = reqPayload.subspan(38, dataLen);
        bool ok = evStorage_.set(name, data);
        lg2::info("SMIF: EV set '{NAME}' ({SZ} bytes) — {RESULT}", "NAME",
                  name, "SZ", dataLen, "RESULT",
                  ok ? "success" : "failed");
        return buildSimpleResponse(reqHdr, response, ok ? 0 : 3);
    }

    // Unknown flags
    return buildSimpleResponse(reqHdr, response, 2);
}

int SmifService::handleGetEvByName(const ChifPktHeader& reqHdr,
                                   std::span<const uint8_t> reqPayload,
                                   std::span<uint8_t> response)
{
    if (reqPayload.size() < maxEvNameLen)
    {
        return buildSimpleResponse(reqHdr, response, 2);
    }

    char nameBuf[maxEvNameLen] = {};
    std::memcpy(nameBuf, reqPayload.data(), maxEvNameLen);
    std::string name(nameBuf, strnlen(nameBuf, sizeof(nameBuf)));

    auto ev = evStorage_.getByName(name);
    if (!ev)
    {
        // Return ErrorCode=1 (not found). The BIOS HandleEvChifErrorResponse
        // function catches this and enters the error recovery path, which
        // skips EV processing and continues POST normally.
        return buildSimpleResponse(reqHdr, response, 1);
    }

    return buildEvDataResponse(reqHdr, response, *ev);
}

int SmifService::handleEvStats(const ChifPktHeader& reqHdr,
                               std::span<uint8_t> response)
{
    // Response: ErrorCode(4) + rem_sz(4) + present_evs(4) + max_sz(4)
    constexpr uint16_t payloadSize = 16;
    constexpr uint16_t respSize =
        static_cast<uint16_t>(sizeof(ChifPktHeader) + payloadSize);

    if (response.size() < respSize)
    {
        return -1;
    }

    initResponse(response, reqHdr, respSize);
    auto resp = responsePayload(response);

    uint32_t errorCode = 0;
    uint32_t remSz = static_cast<uint32_t>(evStorage_.remainingSize());
    uint32_t presentEvs = evStorage_.count();
    uint32_t maxSz = static_cast<uint32_t>(EvStorage::maxSize());

    std::memcpy(resp.data() + 0, &errorCode, sizeof(errorCode));
    std::memcpy(resp.data() + 4, &remSz, sizeof(remSz));
    std::memcpy(resp.data() + 8, &presentEvs, sizeof(presentEvs));
    std::memcpy(resp.data() + 12, &maxSz, sizeof(maxSz));

    return respSize;
}

int SmifService::handleEvState(const ChifPktHeader& reqHdr,
                               std::span<uint8_t> response)
{
    // EV state — always report healthy
    return buildSimpleResponse(reqHdr, response, 0);
}

int SmifService::buildEvDataResponse(const ChifPktHeader& reqHdr,
                                     std::span<uint8_t> response,
                                     const EvEntry& ev)
{
    // Response: ErrorCode(4) + name(32) + sz_ev(2) + data(var)
    uint16_t dataLen = static_cast<uint16_t>(ev.data.size());
    uint16_t payloadSize =
        static_cast<uint16_t>(4 + maxEvNameLen + 2 + dataLen);
    uint16_t respSize =
        static_cast<uint16_t>(sizeof(ChifPktHeader) + payloadSize);

    if (response.size() < respSize)
    {
        return buildSimpleResponse(reqHdr, response, 3);
    }

    initResponse(response, reqHdr, respSize);
    auto resp = responsePayload(response);

    // ErrorCode = 0 (success)
    uint32_t errorCode = 0;
    std::memcpy(resp.data(), &errorCode, sizeof(errorCode));

    // Name (32 bytes, null-padded)
    char nameBuf[maxEvNameLen] = {};
    std::strncpy(nameBuf, ev.name.c_str(), sizeof(nameBuf) - 1);
    std::memcpy(resp.data() + 4, nameBuf, sizeof(nameBuf));

    // sz_ev (2 bytes)
    std::memcpy(resp.data() + 4 + maxEvNameLen, &dataLen, sizeof(dataLen));

    // data
    if (dataLen > 0)
    {
        std::memcpy(resp.data() + 4 + maxEvNameLen + 2, ev.data.data(),
                    dataLen);
    }

    return respSize;
}

} // namespace chif
