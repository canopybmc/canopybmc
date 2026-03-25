// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH

#include "event_logger.hpp"

#include <phosphor-logging/lg2.hpp>

#include <format>
#include <map>
#include <string>

namespace chif
{

namespace
{
constexpr auto loggingService = "xyz.openbmc_project.Logging";
constexpr auto loggingPath = "/xyz/openbmc_project/logging";
constexpr auto loggingCreateIface = "xyz.openbmc_project.Logging.Create";

// HPE severity values
constexpr uint8_t hpeSevInfo = 2;
constexpr uint8_t hpeSevRepaired = 6;
constexpr uint8_t hpeSevCaution = 9;
constexpr uint8_t hpeSevCritical = 15;

// OpenBMC Entry::Level D-Bus enum strings
constexpr auto levelInformational =
    "xyz.openbmc_project.Logging.Entry.Level.Informational";
constexpr auto levelWarning =
    "xyz.openbmc_project.Logging.Entry.Level.Warning";
constexpr auto levelCritical =
    "xyz.openbmc_project.Logging.Entry.Level.Critical";
} // namespace

EventLogger::EventLogger(sdbusplus::bus_t& bus) : bus_(bus) {}

const char* EventLogger::mapSeverity(uint8_t hpeSeverity)
{
    switch (hpeSeverity)
    {
        case hpeSevInfo:
        case hpeSevRepaired:
            return levelInformational;
        case hpeSevCaution:
            return levelWarning;
        case hpeSevCritical:
            return levelCritical;
        default:
            return levelWarning;
    }
}

uint32_t EventLogger::log(uint8_t evtType, uint16_t evtClass, uint16_t evtCode,
                           uint8_t severity, uint16_t matchCode,
                           std::span<const uint8_t> varData)
{
    uint32_t evtNum = nextEvtNum_++;

    auto message =
        std::format("BIOS Event: type=0x{:02X} class=0x{:04X} code=0x{:04X}",
                    evtType, evtClass, evtCode);

    std::map<std::string, std::string> additionalData{
        {"EVT_TYPE", std::format("0x{:02X}", evtType)},
        {"EVT_CLASS", std::format("0x{:04X}", evtClass)},
        {"EVT_CODE", std::format("0x{:04X}", evtCode)},
        {"MATCH_CODE", std::format("0x{:04X}", matchCode)},
        {"HPE_SEVERITY", std::to_string(severity)},
        {"EVT_NUM", std::to_string(evtNum)},
        {"VAR_DATA_LEN", std::to_string(varData.size())},
    };

    const char* level = mapSeverity(severity);

    lg2::info("SMIF 0x0146: {MSG} severity={SEV} evtNum={NUM}", "MSG", message,
              "SEV", severity, "NUM", evtNum);

    try
    {
        auto method = bus_.new_method_call(loggingService, loggingPath,
                                           loggingCreateIface, "Create");
        method.append(message, level, additionalData);
        bus_.call_noreply(method);
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error("Failed to create log entry: {ERR}", "ERR", e.what());
    }

    return evtNum;
}

} // namespace chif
