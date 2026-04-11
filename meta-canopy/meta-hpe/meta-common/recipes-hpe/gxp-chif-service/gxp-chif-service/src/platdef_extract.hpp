// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace chif
{

// Extract and decompress the APML PlatDef blob from the host BIOS SPI flash.
// Searches the "host-prime" MTD partition for the APML firmware volume,
// extracts and decompresses the records. Returns empty vector if not found.
std::vector<uint8_t> extractPlatDef();

// I2C segment mapping: BIOS segment ID → CPLD register + select value.
// Populated from RecordType_I2CEngine (type 14) records in the PlatDef blob.
struct I2cSegmentMapping
{
    uint8_t cpldRegister; // lower byte of CPLD.Byte (mux address: 0x84-0x8B)
    uint8_t channelValue; // CPLD.SelectMask = mux channel select value
};

// Parse decompressed PlatDef records and extract I2C segment→CPLD mappings.
[[nodiscard]] std::unordered_map<uint8_t, I2cSegmentMapping>
    parseI2cSegments(const std::vector<uint8_t>& platDef);

} // namespace chif
