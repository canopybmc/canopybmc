// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#pragma once

#include "ev_storage.hpp"

#include <sdbusplus/bus.hpp>
#include <xyz/openbmc_project/BIOSConfig/Manager/common.hpp>

#include <cstdint>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>

namespace chif
{

inline constexpr std::string_view bootOptionEvPrefix = "CQSBOOT";

class BiosConfigService
{
  public:
    BiosConfigService(sdbusplus::bus_t& bus, EvStorage& evStorage);

    // Extract the display name from a CQSBOOT#### EV payload. Returns nullopt
    // when the payload is too short or holds no printable name.
    static std::optional<std::string> parseBootOptionName(
        std::span<const uint8_t> data);

    // Return the two-digit option id used on D-Bus (the low two digits of the
    // CQSBOOT#### suffix, e.g. "01"), or nullopt if the name is not a
    // boot-option EV.
    static std::optional<std::string> bootOptionId(const std::string& evName);

  private:
    using Manager =
        sdbusplus::common::xyz::openbmc_project::bios_config::Manager;
    using BaseBiosTable = decltype(Manager::properties_t::base_bios_table);
    using BiosAttribute = BaseBiosTable::mapped_type;

    // (Re)build boot-option attributes from the EV store and merge them into
    // the manager's BaseBIOSTable.
    void publish();

    static BiosAttribute makeStringAttribute(const std::string& value);

    BaseBiosTable getBaseTable();
    bool setBaseTable(const BaseBiosTable& table);

    sdbusplus::bus_t& bus_;
    EvStorage& evStorage_;
    // Attribute keys we published last time, so we can refresh only our own
    // entries without disturbing attributes owned by other providers.
    std::set<std::string> managedKeys_;
};

} // namespace chif
