// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#include "health_service.hpp"

#include <algorithm>
#include <cstring>

namespace chif
{

// ---------------------------------------------------------------------------
// HealthService::handle
//
// Default behavior: return a 12-byte response (header + 4-byte ErrorCode=0)
// for every command.  This matches hpe/main behavior and lets BIOS proceed
// through POST.  Real implementations replace individual cases as they
// are added.
// ---------------------------------------------------------------------------
int HealthService::handle(std::span<const uint8_t> request,
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

    std::fill_n(response.data(), respSize, uint8_t{0});
    initResponse(response, hdr, respSize);
    return respSize;
}

} // namespace chif
