// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#include "mdr_bridge.hpp"

#include <phosphor-logging/lg2.hpp>

namespace chif
{

namespace
{
constexpr auto mdrService = "xyz.openbmc_project.Smbios.MDR_V2";
constexpr auto mdrPath = "/xyz/openbmc_project/Smbios/MDR_V2";
constexpr auto mdrInterface = "xyz.openbmc_project.Smbios.MDR_V2";
} // namespace

MdrBridge::MdrBridge(sdbusplus::bus_t& bus) : bus_(bus) {}

bool MdrBridge::synchronize()
{
    try
    {
        auto method = bus_.new_method_call(mdrService, mdrPath, mdrInterface,
                                           "AgentSynchronizeData");

        bus_.call_noreply(method);
        lg2::info("MDR AgentSynchronizeData called successfully");
        return true;
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error("MDR AgentSynchronizeData failed: {ERR}", "ERR", e.what());
        return false;
    }
}

} // namespace chif
