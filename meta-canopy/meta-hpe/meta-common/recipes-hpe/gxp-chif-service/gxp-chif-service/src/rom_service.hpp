// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#pragma once

#include "mdr_bridge.hpp"
#include "packet.hpp"
#include "smbios_writer.hpp"

#include <memory>

namespace chif
{

// ROM command codes (within service_id 0x02)
inline constexpr uint16_t romCmdBegin = 0x0003;
inline constexpr uint16_t romCmdRecord = 0x0004;
inline constexpr uint16_t romCmdEnd = 0x0005;
inline constexpr uint16_t romCmdPowerData = 0x0006;
inline constexpr uint16_t romCmdThermal = 0x0007;
inline constexpr uint16_t romCmdBlob = 0x000C;

// ROM service handler — processes SMBIOS data from BIOS.
//
// Protocol flow (matching HPE reference behavior):
//   1. Host sends cmd 0x03 (begin) — we respond with header-only (8B, no
//      response bit), then clear accumulated records
//   2. Host sends cmd 0x04 (record) or 0x0C (blob) — we append data and
//      respond with header-only (8B, no response bit)
//   3. Host sends cmd 0x05 (end) — we respond with header-only (8B),
//      then finalize and trigger smbios-mdr
//
// Key differences from standard CHIF responses:
//   - ROM responses do NOT set the response bit (0x8000) in the command field
//   - ROM responses are header-only (8 bytes, no ErrorCode payload)
class RomService : public ServiceHandler
{
  public:
    // mdrBridge may be nullptr (for testing without D-Bus).
    RomService(SmbiosWriter& writer, MdrBridge* mdrBridge);

    int handle(std::span<const uint8_t> request,
               std::span<uint8_t> response) override;

    uint8_t serviceId() const override
    {
        return romServiceId;
    }

  private:
    SmbiosWriter& writer_;
    MdrBridge* mdrBridge_;

    int handleBegin(const ChifPktHeader& reqHdr,
                    std::span<uint8_t> response);

    int handleRecord(const ChifPktHeader& reqHdr,
                     std::span<const uint8_t> request,
                     std::span<uint8_t> response);

    int handleEnd(const ChifPktHeader& reqHdr,
                  std::span<uint8_t> response);

    int handleBlob(const ChifPktHeader& reqHdr,
                   std::span<const uint8_t> request,
                   std::span<uint8_t> response);

    // Build a ROM-style response: 8-byte header only, no payload,
    // no response bit. Matches HPE romchf_submit() default behavior.
    static int buildHeaderOnlyResponse(const ChifPktHeader& reqHdr,
                                       std::span<uint8_t> response);
};

} // namespace chif
