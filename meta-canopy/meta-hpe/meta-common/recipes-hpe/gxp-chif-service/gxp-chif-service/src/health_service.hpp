// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#pragma once

#include "packet.hpp"

#include <cstdint>
#include <span>

namespace chif
{

// ---------------------------------------------------------------------------
// Health service command codes (service_id 0x10)
// ---------------------------------------------------------------------------
inline constexpr uint16_t healthCmdNvramSecure = 0x0011;
inline constexpr uint16_t healthCmdResetStatus = 0x0012;
inline constexpr uint16_t healthCmdEncapsulatedIpmi = 0x0015;
inline constexpr uint16_t healthCmdApmlVersion = 0x001C;
inline constexpr uint16_t healthCmdPostFlags = 0x001D;
inline constexpr uint16_t healthCmdLedControl = 0x0114;

// ---------------------------------------------------------------------------
// HealthService — handler for service_id 0x10.
//
// Commands that BIOS expects a response to get one (12 or 15 bytes).
// Commands where the HPE reference returns -1 (no response / silent
// timeout) are handled the same way here.
// ---------------------------------------------------------------------------
class HealthService : public ServiceHandler
{
  public:
    int handle(std::span<const uint8_t> request,
               std::span<uint8_t> response) override;

    uint8_t serviceId() const override
    {
        return healthServiceId;
    }
};

} // namespace chif
