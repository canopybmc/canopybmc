// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#pragma once

#include "ev_storage.hpp"
#include "packet.hpp"

#include <cstdint>
#include <span>
#include <unordered_map>

namespace chif
{

// ---------------------------------------------------------------------------
// SMIF command codes (service_id 0x00)
// ---------------------------------------------------------------------------

// Hardware info
inline constexpr uint16_t smifCmdHwRevision = 0x0002;
inline constexpr uint16_t smifCmdFlashInfo = 0x0050;
inline constexpr uint16_t smifCmdDateTime = 0x0055;

// Network
inline constexpr uint16_t smifCmdIPv4Config = 0x0006;
inline constexpr uint16_t smifCmdNicConfig = 0x0063;
inline constexpr uint16_t smifCmdIPv6Config = 0x0120;

// BIOS readiness / lifecycle
inline constexpr uint16_t smifCmdBiosReady = 0x0008;
inline constexpr uint16_t smifCmdBiosSync = 0x0136;
inline constexpr uint16_t smifCmdSecurityStateGet = 0x0139;
inline constexpr uint16_t smifCmdPostState = 0x0143;
inline constexpr uint16_t smifCmdSecurityStateSet = 0x0158;
inline constexpr uint16_t smifCmdBootProgress = 0x0209;

// PCI / hardware
inline constexpr uint16_t smifCmdPciDeviceInfo = 0x0035;
inline constexpr uint16_t smifCmdI2cTransaction = 0x0072;
inline constexpr uint16_t smifCmdGpioCpld = 0x0088;
inline constexpr uint16_t smifCmdLicenseKey = 0x006E;
inline constexpr uint16_t smifCmdFieldAccess = 0x0153;
inline constexpr uint16_t smifCmdPowerRegulator = 0x0176;

// EV storage
inline constexpr uint16_t smifCmdGetEvByIndex = 0x012B;
inline constexpr uint16_t smifCmdSetDeleteEv = 0x012C;
inline constexpr uint16_t smifCmdGetEvAuthStatus = 0x012D;
inline constexpr uint16_t smifCmdGetEvByName = 0x0130;
inline constexpr uint16_t smifCmdEvStats = 0x0132;
inline constexpr uint16_t smifCmdEvState = 0x0133;

// EV operation flags (for 0x012C SetDeleteEv)
inline constexpr uint8_t evFlagSet = 0x01;
inline constexpr uint8_t evFlagDelete = 0x02;
inline constexpr uint8_t evFlagDeleteAll = 0x04;

// Event logging
inline constexpr uint16_t smifCmdQuickEventLog = 0x0146;

// PlatDef
inline constexpr uint16_t smifCmdPlatDefUpload = 0x0200;
inline constexpr uint16_t smifCmdPlatDefDownload = 0x0202;
inline constexpr uint16_t smifCmdPlatDefV2Begin = 0x0203;
inline constexpr uint16_t smifCmdPlatDefV2Chunk = 0x0204;
inline constexpr uint16_t smifCmdPlatDefV2Query = 0x0205;
inline constexpr uint16_t smifCmdPlatDefV2Download = 0x0206;
inline constexpr uint16_t smifCmdPlatDefV2End = 0x0207;

// ---------------------------------------------------------------------------
// SmifService — handler for service_id 0x00.
// ---------------------------------------------------------------------------
class SmifService : public ServiceHandler
{
  public:
    explicit SmifService(EvStorage* evStorage = nullptr,
                        std::unordered_map<uint8_t, int> segmentBusMap = {});

    int handle(std::span<const uint8_t> request,
               std::span<uint8_t> response) override;

    uint8_t serviceId() const override
    {
        return smifServiceId;
    }

  private:
    // EV command handlers
    int handleGetEvByIndex(const ChifPktHeader& hdr,
                           std::span<const uint8_t> reqPayload,
                           std::span<uint8_t> response);
    int handleSetDeleteEv(const ChifPktHeader& hdr,
                          std::span<const uint8_t> reqPayload,
                          std::span<uint8_t> response);
    int handleGetEvByName(const ChifPktHeader& hdr,
                          std::span<const uint8_t> reqPayload,
                          std::span<uint8_t> response);
    int handleEvStats(const ChifPktHeader& hdr,
                      std::span<uint8_t> response);
    int handleEvState(const ChifPktHeader& hdr,
                      std::span<uint8_t> response);
    int handleGetEvAuthStatus(const ChifPktHeader& hdr,
                              std::span<uint8_t> response);

    // I2C proxy handler
    int handleI2cProxy(const ChifPktHeader& hdr,
                       std::span<const uint8_t> reqPayload,
                       std::span<uint8_t> response);

    // PlatDef v1 download handler
    int handlePlatDefDownload(const ChifPktHeader& hdr,
                              std::span<const uint8_t> reqPayload,
                              std::span<uint8_t> response);

    // Response helpers
    static int buildSimpleResponse(const ChifPktHeader& hdr,
                                   std::span<uint8_t> response,
                                   uint32_t errorCode);
    static int buildEvDataResponse(const ChifPktHeader& hdr,
                                   std::span<uint8_t> response,
                                   const EvEntry& ev);

    EvStorage* evStorage_;
    std::unordered_map<uint8_t, int> segmentToBus_;
};

} // namespace chif
