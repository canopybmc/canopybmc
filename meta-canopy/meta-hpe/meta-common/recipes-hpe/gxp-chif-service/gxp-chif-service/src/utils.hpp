// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#pragma once

#include <sdbusplus/bus.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace chif
{

inline constexpr auto propertiesInterface = "org.freedesktop.DBus.Properties";

template <typename T>
std::optional<T> getProperty(sdbusplus::bus_t& bus, const char* service,
                             const char* path, const char* interface,
                             const char* property)
{
    try
    {
        auto m = bus.new_method_call(service, path, propertiesInterface, "Get");
        m.append(interface, property);
        std::variant<T> value;
        bus.call(m).read(value);
        return std::get<T>(value);
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
}

template <typename T>
bool setProperty(sdbusplus::bus_t& bus, const char* service, const char* path,
                 const char* interface, const char* property, const T& value)
{
    try
    {
        auto m = bus.new_method_call(service, path, propertiesInterface, "Set");
        m.append(interface, property, std::variant<T>(value));
        bus.call(m);
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

std::optional<uint32_t> parseV4(const std::string& s);
std::optional<std::array<uint8_t, 16>> parseV6(const std::string& s);
std::optional<std::array<uint8_t, 6>> parseMac(std::string_view s);
uint32_t prefixToMask4(uint8_t prefix);

} // namespace chif
