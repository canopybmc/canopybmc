// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#pragma once

#include <cstdint>
#include <istream>
#include <vector>

namespace chif
{

// Minimal EFI GUID representation for firmware volume identification.
#pragma pack(push, 1)
struct EfiGuid
{
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t data4[8];
};
#pragma pack(pop)

// Extract the raw payload of an FFS file from a UEFI firmware image.
// Scans every 64 KB for a firmware volume whose extended header name
// matches fvGuid, then walks FFS entries for a file matching fileGuid.
// Returns the file's section data, or an empty vector if not found.
[[nodiscard]] std::vector<uint8_t> extractFfsFile(std::istream& rom,
                                                   size_t romSize,
                                                   const EfiGuid& fvGuid,
                                                   const EfiGuid& fileGuid);

} // namespace chif
