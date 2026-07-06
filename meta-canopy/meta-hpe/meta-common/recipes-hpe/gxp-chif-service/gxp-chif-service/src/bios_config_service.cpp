// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#include "bios_config_service.hpp"

#include "utils.hpp"

#include <phosphor-logging/lg2.hpp>

#include <utility>

namespace chif
{

namespace
{
constexpr auto biosConfigService = "xyz.openbmc_project.BIOSConfigManager";

uint16_t readLe16(std::span<const uint8_t> data, size_t offset)
{
    return static_cast<uint16_t>(data[offset]) |
           static_cast<uint16_t>(static_cast<uint16_t>(data[offset + 1]) << 8);
}

} // namespace

BiosConfigService::BiosConfigService(sdbusplus::bus_t& bus,
                                     EvStorage& evStorage) :
    bus_(bus), evStorage_(evStorage)
{
    // Refresh whenever a boot-option EV changes (or on a bulk clear). Other
    // EV writes (e.g. CQHBOOTORDER from BootService) are ignored to avoid
    // needless republishing.
    evStorage_.addChangeCallback([this](std::string_view name) {
        if (name.empty() || name.starts_with(bootOptionEvPrefix))
        {
            publish();
        }
    });

    publish();
}

std::optional<std::string> BiosConfigService::parseBootOptionName(
    std::span<const uint8_t> data)
{
    if (data.size() < 16)
    {
        return std::nullopt;
    }

    uint16_t bootStrLen = readLe16(data, 14);
    size_t nameOffset = 16 + bootStrLen + 6 + 2; // past structuredBootStr,
                                                 // reserved, uefi len field
    if (nameOffset >= data.size())
    {
        return std::nullopt;
    }

    std::string name;
    for (size_t i = nameOffset; i + 1 < data.size(); i += 2)
    {
        uint16_t ch = readLe16(data, i);
        if (ch == 0)
        {
            break;
        }
        // Boot option names are usually ASCII. Remove anything
        // outside of printable ASCII rather than mangling the UTF-16.
        name.push_back((ch >= 0x20 && ch < 0x7f) ? static_cast<char>(ch) : '?');
    }

    if (name.empty())
    {
        return std::nullopt;
    }
    return name;
}

std::optional<std::string> BiosConfigService::bootOptionId(
    const std::string& evName)
{
    if (!evName.starts_with(bootOptionEvPrefix))
    {
        return std::nullopt;
    }
    // The EV name is CQSBOOT#### (4 hex digits), but only the low two digits
    // identify the option on D-Bus.
    std::string digits = evName.substr(bootOptionEvPrefix.size());
    if (digits.size() < 4)
    {
        return std::nullopt;
    }
    return digits.substr(2, 2);
}

BiosConfigService::BiosAttribute BiosConfigService::makeStringAttribute(
    const std::string& value)
{
    return BiosAttribute{
        Manager::AttributeType::String,
        true,
        value,
        "HPE UEFI boot option",
        "Boot/BootOrder",
        value,
        value,
        {},
    };
}

void BiosConfigService::publish()
{
    lg2::debug("BIOS config: rebuilding boot options from {COUNT} EV(s)",
               "COUNT", evStorage_.count());

    BaseBiosTable ours;
    for (uint32_t i = 0; i < evStorage_.count(); ++i)
    {
        auto entry = evStorage_.getByIndex(i);
        if (!entry)
        {
            continue;
        }

        auto id = bootOptionId(entry->name);
        if (!id)
        {
            lg2::debug("BIOS config: skipping non-boot EV {EV}", "EV",
                       entry->name);
            continue;
        }

        auto name = parseBootOptionName(entry->data);
        if (!name)
        {
            lg2::warning("BIOS config: cannot parse boot option {EV}", "EV",
                         entry->name);
            continue;
        }

        lg2::debug("BIOS config: Boot{ID} = '{NAME}' ({BYTES} bytes)", "ID",
                   *id, "NAME", *name, "BYTES", entry->data.size());
        ours.emplace("Boot" + *id, makeStringAttribute(*name));
    }

    BaseBiosTable table = getBaseTable();
    for (const auto& key : managedKeys_)
    {
        table.erase(key);
    }
    managedKeys_.clear();
    for (auto& [key, value] : ours)
    {
        managedKeys_.insert(key);
        table[key] = std::move(value);
    }

    if (!setBaseTable(table))
    {
        return;
    }

    lg2::info("BIOS config: published {COUNT} boot option(s)", "COUNT",
              managedKeys_.size());
}

BiosConfigService::BaseBiosTable BiosConfigService::getBaseTable()
{
    auto value = getProperty<BaseBiosTable>(
        bus_, biosConfigService, Manager::instance_path, Manager::interface,
        Manager::property_names::base_bios_table);
    if (!value)
    {
        lg2::warning("BIOS config: cannot read BaseBIOSTable, starting empty");
        return {};
    }
    return *value;
}

bool BiosConfigService::setBaseTable(const BaseBiosTable& table)
{
    if (!setProperty(bus_, biosConfigService, Manager::instance_path,
                     Manager::interface,
                     Manager::property_names::base_bios_table, table))
    {
        lg2::error("BIOS config: failed to set BaseBIOSTable");
        return false;
    }
    return true;
}

} // namespace chif
