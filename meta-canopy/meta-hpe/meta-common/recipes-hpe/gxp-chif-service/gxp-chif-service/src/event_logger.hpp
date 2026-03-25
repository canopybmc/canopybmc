// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#pragma once

#include <cstdint>
#include <span>

#include <sdbusplus/bus.hpp>

namespace chif
{

// Forwards BIOS hardware events to phosphor-logging via D-Bus.
//
// The BIOS reports events (memory errors, CPU failures, fan faults, thermal
// warnings) over CHIF using SMIF command 0x0146. This class maps HPE severity
// levels to OpenBMC Entry::Level values and calls the phosphor-logging Create
// method to persist them in the Redfish EventLog.
class EventLogger
{
  public:
    explicit EventLogger(sdbusplus::bus_t& bus);

    // Log a BIOS event to phosphor-logging.
    // Returns a per-session event number (resets on service restart; the BIOS
    // does not rely on cross-restart monotonicity).
    uint32_t log(uint8_t evtType, uint16_t evtClass, uint16_t evtCode,
                 uint8_t severity, uint16_t matchCode,
                 std::span<const uint8_t> varData);

    // Map HPE severity byte to OpenBMC severity string.
    // Returns the D-Bus enum string for Entry::Level.
    static const char* mapSeverity(uint8_t hpeSeverity);

  private:
    sdbusplus::bus_t& bus_;
    uint32_t nextEvtNum_ = 1; // Single-threaded access only (daemon main loop)
};

} // namespace chif
