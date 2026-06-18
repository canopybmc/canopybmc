// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Encoder for the IPMI Platform Management FRU Information Storage Definition
// v1.0. Produces a binary FRU image (Common Header + optional Board area +
// optional Product area) suitable for serving via xyz.openbmc_project.FruDevice
// and the stock IPMI storage handlers.
namespace fru
{

// Chassis Info Area. The numeric `type` is the SMBIOS System Enclosure type
// (e.g. 23 = Rack Mount Chassis, 17 = Main Server Chassis). dbus-sdr assigns
// IPMI FRU device id 0 to a FRU whose chassis type is rack-mount or main-server,
// so the baseboard FRU should carry one of those.
struct ChassisInfo
{
    uint8_t type = 0;
    std::optional<std::string> partNumber;
    std::optional<std::string> serialNumber;
    std::vector<std::string> custom;
};

// String fields for the Board Info Area, in spec order. Unset/empty values are
// encoded as zero-length ASCII fields.
struct BoardInfo
{
    std::optional<std::string> manufacturer;
    std::optional<std::string> productName;
    std::optional<std::string> serialNumber;
    std::optional<std::string> partNumber;
    std::optional<std::string> fruFileId;
    std::vector<std::string> custom;
};

// String fields for the Product Info Area, in spec order.
struct ProductInfo
{
    std::optional<std::string> manufacturer;
    std::optional<std::string> productName;
    std::optional<std::string> partNumber; // a.k.a. model/part number
    std::optional<std::string> version;
    std::optional<std::string> serialNumber;
    std::optional<std::string> assetTag;
    std::optional<std::string> fruFileId;
    std::vector<std::string> custom;
};

// Build a complete FRU image. Each present area is padded to a multiple of 8
// bytes and terminated with a valid checksum; the common header offsets and
// checksum are filled accordingly. Returns the binary blob.
std::vector<uint8_t> build(const std::optional<ChassisInfo>& chassis,
                           const std::optional<BoardInfo>& board,
                           const std::optional<ProductInfo>& product);

} // namespace fru
