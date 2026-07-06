// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#pragma once

#include "ev_storage.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <xyz/openbmc_project/Control/Boot/Mode/common.hpp>
#include <xyz/openbmc_project/Control/Boot/Source/common.hpp>

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace chif
{
inline constexpr auto bootOrderEvName = "CQHBOOTORDER";
inline constexpr auto bootNextEvName = "CQTBOOTNEXT";

class BootService
{
  public:
    using Source =
        sdbusplus::common::xyz::openbmc_project::control::boot::Source;
    using Sources = Source::Sources;
    using Mode = sdbusplus::common::xyz::openbmc_project::control::boot::Mode;
    using Modes = Mode::Modes;

    BootService(sdbusplus::bus_t& bus, EvStorage& evStorage);

    // Parse the 16-bit UEFI option number out of a CQSBOOT#### EV name.
    // Returns nullopt if the name is not a boot-option EV.
    static std::optional<uint16_t> optionNumber(std::string_view evName);

    // Priority-ordered display-name keywords to match for a given boot target.
    // This is kinda hacky mainly because I couldn't figure out if there's a
    // common identifier that could be used to filter the boot values.
    static std::vector<std::string_view> targetKeywords(Sources source,
                                                        Modes mode);

    // Return the option number of the first (option, displayName) pair whose
    // name contains any keyword, tried in keyword priority order.
    static std::optional<uint16_t> matchOption(
        const std::vector<std::pair<uint16_t, std::string>>& options,
        const std::vector<std::string_view>& keywords);

    // Parse the existing CQHBOOTORDER array and move `option` to the front
    // (adding it if absent). Returns a byte blob.
    static std::vector<uint8_t> reorderBootOrder(
        std::span<const uint8_t> current, uint16_t option);

    // Encode a single option number as a uint16, mainly used for CQTBOOTNEXT.
    static std::vector<uint8_t> encodeOption(uint16_t option);

  private:
    void syncEffective();

    // Resolve the active phosphor target to a boot-option number by matching
    // the CQSBOOT#### display names in the EV store. nullopt = no override.
    std::optional<uint16_t> resolveTarget(Sources source, Modes mode);

    bool applyOneTime(uint16_t option);
    bool applyPersistent(uint16_t option);
    void clearOneTime();

    void onEvChanged(std::string_view name);
    void clearOverrideSettings();

    Sources getBootSource(const std::string& path);
    Modes getBootMode(const std::string& path);
    bool getOneTimeEnabled();

    sdbusplus::bus_t& bus_;
    EvStorage& evStorage_;
    std::vector<sdbusplus::bus::match_t> matches_;
    // Set while BootService itself mutates the EV store, so its own writes are
    // not misread as a host-side consumption in onEvChanged().
    bool applyingOverride_ = false;
};

} // namespace chif
