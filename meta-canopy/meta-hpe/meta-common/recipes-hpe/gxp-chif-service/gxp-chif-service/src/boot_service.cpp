// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#include "boot_service.hpp"

#include "bios_config_service.hpp"
#include "utils.hpp"

#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/Object/Enable/common.hpp>

#include <charconv>

namespace chif
{

namespace
{
namespace rules = sdbusplus::bus::match::rules;

using Enable = sdbusplus::common::xyz::openbmc_project::object::Enable;

constexpr auto settingsService = "xyz.openbmc_project.Settings";
constexpr auto bootPath = "/xyz/openbmc_project/control/host0/boot";
constexpr auto oneTimeBootPath =
    "/xyz/openbmc_project/control/host0/boot/one_time";

} // namespace

BootService::BootService(sdbusplus::bus_t& bus, EvStorage& evStorage) :
    bus_(bus), evStorage_(evStorage)
{
    using Source = BootService::Source;
    using Mode = BootService::Mode;

    matches_.emplace_back(bus_,
                          rules::propertiesChanged(bootPath, Source::interface),
                          [this](sdbusplus::message_t&) { syncEffective(); });
    matches_.emplace_back(bus_,
                          rules::propertiesChanged(bootPath, Mode::interface),
                          [this](sdbusplus::message_t&) { syncEffective(); });
    matches_.emplace_back(
        bus_, rules::propertiesChanged(oneTimeBootPath, Enable::interface),
        [this](sdbusplus::message_t&) { syncEffective(); });

    // The host deletes CQTBOOTNEXT once it consumes the one-time boot
    evStorage_.addChangeCallback([this](std::string_view name) {
        onEvChanged(name);
    });

    syncEffective();
}

std::optional<uint16_t> BootService::optionNumber(std::string_view evName)
{
    if (!evName.starts_with(bootOptionEvPrefix))
    {
        return std::nullopt;
    }
    auto digits = evName.substr(bootOptionEvPrefix.size());
    if (digits.size() != 4)
    {
        return std::nullopt;
    }
    uint16_t value = 0;
    auto* end = digits.data() + digits.size();
    auto [ptr, ec] = std::from_chars(digits.data(), end, value, 16);
    if (ec != std::errc{} || ptr != end)
    {
        return std::nullopt;
    }
    return value;
}

std::vector<std::string_view> BootService::targetKeywords(Sources source,
                                                          Modes mode)
{
    // BIOS setup is requested through Boot.Mode and takes precedence.
    if (mode == Modes::Setup)
    {
        return {"System Utilities", "Setup"};
    }

    switch (source)
    {
        case Sources::Network:
            return {"Network Boot", "PXE"};
        case Sources::HTTP:
            return {"HTTP Boot", "HTTP"};
        case Sources::RemovableMedia:
            return {"USB"};
        case Sources::ExternalMedia:
            return {"Virtual Media", "Virtual CD", "Virtual", "CD/DVD", "CD",
                    "DVD",           "Optical",    "Blu-ray"};
        case Sources::Disk:
            return {"Logical Drive", "Bay", "NVMe", "SATA", "SAS",
                    "SSD",           "HDD", "RAID", "Hard", "Disk"};
        case Sources::Default:
            break;
    }
    return {};
}

std::optional<uint16_t> BootService::matchOption(
    const std::vector<std::pair<uint16_t, std::string>>& options,
    const std::vector<std::string_view>& keywords)
{
    for (auto keyword : keywords)
    {
        for (const auto& [number, name] : options)
        {
            if (name.contains(keyword))
            {
                return number;
            }
        }
    }
    return std::nullopt;
}

std::vector<uint8_t> BootService::reorderBootOrder(
    std::span<const uint8_t> current, uint16_t option)
{
    std::vector<uint16_t> order;
    order.reserve(current.size() / 2);
    for (size_t i = 0; i + 1 < current.size(); i += 2)
    {
        order.push_back(static_cast<uint16_t>(
            current[i] | (static_cast<uint16_t>(current[i + 1]) << 8)));
    }

    std::erase(order, option);
    order.insert(order.begin(), option);

    std::vector<uint8_t> out;
    out.reserve(order.size() * 2);
    for (uint16_t v : order)
    {
        out.push_back(static_cast<uint8_t>(v & 0xFF));
        out.push_back(static_cast<uint8_t>(v >> 8));
    }
    return out;
}

std::vector<uint8_t> BootService::encodeOption(uint16_t option)
{
    return {static_cast<uint8_t>(option & 0xFF),
            static_cast<uint8_t>(option >> 8)};
}

std::optional<uint16_t> BootService::resolveTarget(Sources source, Modes mode)
{
    auto keywords = targetKeywords(source, mode);
    if (keywords.empty())
    {
        return std::nullopt;
    }

    std::vector<std::pair<uint16_t, std::string>> options;
    for (uint32_t i = 0; i < evStorage_.count(); ++i)
    {
        auto entry = evStorage_.getByIndex(i);
        if (!entry)
        {
            continue;
        }
        auto number = optionNumber(entry->name);
        if (!number)
        {
            continue;
        }
        auto name = BiosConfigService::parseBootOptionName(entry->data);
        if (!name)
        {
            continue;
        }
        options.emplace_back(*number, std::move(*name));
    }

    auto option = matchOption(options, keywords);
    if (!option)
    {
        lg2::warning("Boot override: no boot option matches target "
                     "(source={SRC} mode={MODE})",
                     "SRC", Source::convertSourcesToString(source), "MODE",
                     Mode::convertModesToString(mode));
    }
    return option;
}

bool BootService::applyOneTime(uint16_t option)
{
    if (!evStorage_.set(bootNextEvName, encodeOption(option)))
    {
        lg2::error("One-time boot: failed to persist {EV}", "EV",
                   bootNextEvName);
        return false;
    }
    lg2::info("One-time boot: option {OPT}", "OPT", lg2::hex, option);
    return true;
}

bool BootService::applyPersistent(uint16_t option)
{
    auto entry = evStorage_.getByName(bootOrderEvName);
    if (!entry || entry->data.size() < 2)
    {
        lg2::error("Persistent boot: {EV} missing; cannot reorder "
                   "(host must boot once to publish it)",
                   "EV", bootOrderEvName);
        return false;
    }

    auto payload = reorderBootOrder(entry->data, option);
    if (!evStorage_.set(bootOrderEvName, payload))
    {
        lg2::error("Persistent boot: Failed to persist {EV}", "EV",
                   bootOrderEvName);
        return false;
    }
    lg2::info("Persistent boot: option {OPT} moved to front", "OPT", lg2::hex,
              option);
    return true;
}

void BootService::clearOneTime()
{
    if (evStorage_.getByName(bootNextEvName))
    {
        applyingOverride_ = true;
        bool ok = evStorage_.del(bootNextEvName);
        applyingOverride_ = false;
        if (ok)
        {
            lg2::info("One-time boot override cleared");
        }
        else
        {
            lg2::error("Failed to clear {EV}", "EV", bootNextEvName);
        }
    }
}

void BootService::onEvChanged(std::string_view name)
{
    if (applyingOverride_)
    {
        return; // our own EV write
    }
    if (!name.empty() && name != bootNextEvName)
    {
        return; // unrelated EV
    }
    if (evStorage_.getByName(bootNextEvName))
    {
        return; // not consumed yet
    }
    if (!getOneTimeEnabled())
    {
        return; // no active one-time override to reconcile
    }

    lg2::info("One-time boot consumed by host! Clearing override in settings");
    clearOverrideSettings();
}

void BootService::clearOverrideSettings()
{
    setProperty<Modes>(bus_, settingsService, bootPath, Mode::interface,
                       Mode::property_names::boot_mode, Modes::Regular);
    setProperty<Sources>(bus_, settingsService, bootPath, Source::interface,
                         Source::property_names::boot_source, Sources::Default);
    setProperty<bool>(bus_, settingsService, oneTimeBootPath, Enable::interface,
                      Enable::property_names::enabled, false);
}

void BootService::syncEffective()
{
    const bool oneTime = getOneTimeEnabled();
    const Sources source = getBootSource(bootPath);
    const Modes mode = getBootMode(bootPath);

    lg2::info("Boot settings sync: oneTime={ONE} source={SRC} mode={MODE}",
              "ONE", oneTime, "SRC", Source::convertSourcesToString(source),
              "MODE", Mode::convertModesToString(mode));

    auto option = resolveTarget(source, mode);
    if (!option)
    {
        // drop any pending one-time selection. The persistent order
        // is left untouched (we cannot reconstruct the ROM's
        // original order, and it re-publishes it every boot anyway).
        clearOneTime();
        return;
    }

    if (oneTime)
    {
        applyOneTime(*option);
    }
    else
    {
        // A persistent selection supersedes a stale one-time entry.
        clearOneTime();
        applyPersistent(*option);
    }
}

BootService::Sources BootService::getBootSource(const std::string& path)
{
    auto value = getProperty<Sources>(bus_, settingsService, path.c_str(),
                                      Source::interface,
                                      Source::property_names::boot_source);
    if (!value)
    {
        lg2::error("Failed to read BootSource at {PATH}", "PATH", path);
        return Sources::Default;
    }
    return *value;
}

BootService::Modes BootService::getBootMode(const std::string& path)
{
    auto value =
        getProperty<Modes>(bus_, settingsService, path.c_str(), Mode::interface,
                           Mode::property_names::boot_mode);
    if (!value)
    {
        lg2::error("Failed to read BootMode at {PATH}", "PATH", path);
        return Modes::Regular;
    }
    return *value;
}

bool BootService::getOneTimeEnabled()
{
    auto value =
        getProperty<bool>(bus_, settingsService, oneTimeBootPath,
                          Enable::interface, Enable::property_names::enabled);
    if (!value)
    {
        lg2::error("Failed to read one-time Enabled");
    }
    return value.value_or(false);
}

} // namespace chif
