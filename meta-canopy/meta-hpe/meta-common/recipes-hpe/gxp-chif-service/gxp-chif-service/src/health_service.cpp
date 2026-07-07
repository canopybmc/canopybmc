// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#include "health_service.hpp"

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

    return emitResponse(parseHeader(request), response, uint32_t{0});
}

} // namespace chif
