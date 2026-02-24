// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#pragma once

#include <sdbusplus/bus.hpp>

namespace chif
{

// Calls AgentSynchronizeData() on the smbios-mdr service to trigger
// parsing of the SMBIOS data file.
class MdrBridge
{
  public:
    explicit MdrBridge(sdbusplus::bus_t& bus);

    // Trigger smbios-mdr to re-read /var/lib/smbios/smbios2.
    // Returns true if the D-Bus call succeeded.
    bool synchronize();

  private:
    sdbusplus::bus_t& bus_;
};

} // namespace chif
