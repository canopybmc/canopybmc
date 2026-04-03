// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#include "smif_service.hpp"

#include <phosphor-logging/lg2.hpp>

#include <algorithm>
#include <cstring>

namespace chif
{

// ---------------------------------------------------------------------------
// SmifService::handle
//
// Default behavior: return a 12-byte response (header + 4-byte ErrorCode)
// for every command.  EV enumeration commands MUST return ErrorCode=1
// ("not found") or BIOS will try to parse empty data and hang.
// All other commands return ErrorCode=0 (success).
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

    // Determine ErrorCode: EV read commands must return "not found" (1)
    // to prevent BIOS from parsing empty/zero payload as valid EV data.
    uint32_t errorCode = 0;
    switch (hdr.command)
    {
        case smifCmdGetEvByIndex:
        case smifCmdGetEvAuthStatus:
        case smifCmdGetEvByName:
            errorCode = 1;
            break;
        default:
            errorCode = 0;
            break;
    }

    initResponse(response, hdr, respSize);
    auto resp = responsePayload(response);
    std::memcpy(resp.data(), &errorCode, sizeof(errorCode));
    return respSize;
}

} // namespace chif
