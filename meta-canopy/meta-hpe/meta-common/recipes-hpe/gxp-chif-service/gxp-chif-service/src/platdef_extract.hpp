// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#pragma once

#include <cstdint>
#include <vector>

namespace chif
{

// Extract and decompress the APML PlatDef blob from the host BIOS SPI flash.
// Searches the "host-prime" MTD partition for the APML firmware volume,
// extracts and decompresses the records. Returns empty vector if not found.
std::vector<uint8_t> extractPlatDef();

} // namespace chif
