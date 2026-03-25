// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#pragma once

#include "ev_storage.hpp"
#include "event_logger.hpp"
#include "packet.hpp"

#include <string>
#include <vector>

namespace chif
{

// SMIF command codes — platform info
inline constexpr uint16_t smifCmdHwRevision = 0x0002;
inline constexpr uint16_t smifCmdFlashInfo = 0x0050;
inline constexpr uint16_t smifCmdDateTime = 0x0055;

// SMIF command codes — EV storage
inline constexpr uint16_t smifCmdGetEvByIndex = 0x012B;
inline constexpr uint16_t smifCmdSetDeleteEv = 0x012C;
inline constexpr uint16_t smifCmdGetEvAuthStatus = 0x012D;
inline constexpr uint16_t smifCmdGetEvByName = 0x0130;
inline constexpr uint16_t smifCmdEvStats = 0x0132;
inline constexpr uint16_t smifCmdEvState = 0x0133;

// SMIF command codes — Sprint 2: Core BIOS integration
inline constexpr uint16_t smifCmdLicenseKey = 0x006E;
inline constexpr uint16_t smifCmdBiosSync = 0x0136;
inline constexpr uint16_t smifCmdSecurityStateGet = 0x0139;
inline constexpr uint16_t smifCmdPostState = 0x0143;
inline constexpr uint16_t smifCmdFieldAccess = 0x0153;
inline constexpr uint16_t smifCmdSecurityStateSet = 0x0158;
inline constexpr uint16_t smifCmdBootProgress = 0x0209;

// SMIF command codes — Sprint 3: Network & Hardware
inline constexpr uint16_t smifCmdIPv4Config = 0x0006;
inline constexpr uint16_t smifCmdPciDeviceInfo = 0x0035;
inline constexpr uint16_t smifCmdNicConfig = 0x0063;
inline constexpr uint16_t smifCmdIPv6Config = 0x0120;

// SMIF command codes — Sprint 4: Hardware access
inline constexpr uint16_t smifCmdGpioCpld = 0x0088;
inline constexpr uint16_t smifCmdPowerRegulator = 0x0176;
inline constexpr uint16_t smifCmdPlatDefUpload = 0x0200;
inline constexpr uint16_t smifCmdPlatDefDownload = 0x0202;

// PlatDef v2 protocol (0x0203-0x0207): used by newer BIOS versions
// BIOS uploads PlatDef blob via 0x0203(begin)+0x0204(chunk)+0x0207(end),
// then reads specific records via 0x0205(query)+0x0206(download).
inline constexpr uint16_t smifCmdPlatDefV2Begin = 0x0203;
inline constexpr uint16_t smifCmdPlatDefV2Chunk = 0x0204;
inline constexpr uint16_t smifCmdPlatDefV2Query = 0x0205;
inline constexpr uint16_t smifCmdPlatDefV2Download = 0x0206;
inline constexpr uint16_t smifCmdPlatDefV2End = 0x0207;

// SMIF command codes — I2C proxy
inline constexpr uint16_t smifCmdI2cTransaction = 0x0072;

// I2C proxy response size: header(8) + payload(49) = 57 bytes.
// The BIOS validates Size==0x39 && Cmd==0x8072 && Sid==0x00.
inline constexpr uint16_t i2cResponseSize = 57;
inline constexpr uint16_t i2cPayloadSize = 49;

// I2C proxy error codes
inline constexpr uint32_t i2cSuccess = 0;
inline constexpr uint32_t i2cBadArgument = 3;
inline constexpr uint32_t i2cSegmentDoesNotExist = 108;

// SMIF command codes — misc
inline constexpr uint16_t smifCmdBiosReady = 0x0008;
inline constexpr uint16_t smifCmdQuickEventLog = 0x0146;

// Health service command codes (SID 0x10)
inline constexpr uint16_t healthCmdNvramSecure = 0x0011;
inline constexpr uint16_t healthCmdResetStatus = 0x0012;
inline constexpr uint16_t healthCmdLedControl = 0x0114;
inline constexpr uint16_t healthCmdEncapsulatedIpmi = 0x0015;
inline constexpr uint16_t healthCmdApmlVersion = 0x001C;
inline constexpr uint16_t healthCmdPostFlags = 0x001D;

// EV Set/Delete flags (in pkt_012c.flags)
inline constexpr uint8_t evFlagSet = 0x01;
inline constexpr uint8_t evFlagDelete = 0x02;
inline constexpr uint8_t evFlagDeleteAll = 0x04;

// SMIF service handler — responds to BIOS commands including EV storage,
// platform info (hardware revision, date/time, flash info), and event logging.
//
// EV (Environment Variable) commands allow the BIOS to persistently store
// and retrieve configuration settings (boot order, security config, etc.)
// via SMIF commands 0x012B/0x012C/0x0130/0x0132/0x0133.
//
// Platform info commands (0x0002, 0x0050, 0x0055) report BMC firmware version,
// hardware revision, date/time, and flash details to the BIOS during POST.
class SmifService : public ServiceHandler
{
  public:
    // Construct with EV storage and optional event logger.
    // osReleasePath allows overriding /etc/os-release for testing.
    explicit SmifService(EvStorage& evStorage,
                         EventLogger* eventLogger = nullptr,
                         const std::string& osReleasePath = "/etc/os-release",
                         const std::string& serialNumberPath =
                             "/proc/device-tree/serial-number");

    int handle(std::span<const uint8_t> request,
               std::span<uint8_t> response) override;

    uint8_t serviceId() const override
    {
        return smifServiceId;
    }

  private:
    EvStorage& evStorage_;
    EventLogger* eventLogger_ = nullptr;

    // Cached platform info (read once at construction)
    std::string versionId_;     // e.g., "3.0.0-dev"
    std::string buildId_;       // e.g., "2026-03-19"
    std::string boardSerial_;   // e.g., "CZUD2102YJ"
    std::string productId_;     // e.g., "P71673-425" (from device tree)
    uint16_t fwVersionPacked_;  // e.g., 0x0300

    // EV command handlers
    int handleGetEvByIndex(const ChifPktHeader& reqHdr,
                           std::span<const uint8_t> reqPayload,
                           std::span<uint8_t> response);

    int handleSetDeleteEv(const ChifPktHeader& reqHdr,
                          std::span<const uint8_t> reqPayload,
                          std::span<uint8_t> response);

    int handleGetEvByName(const ChifPktHeader& reqHdr,
                          std::span<const uint8_t> reqPayload,
                          std::span<uint8_t> response);

    int handleEvStats(const ChifPktHeader& reqHdr,
                      std::span<uint8_t> response);

    int handleEvState(const ChifPktHeader& reqHdr,
                      std::span<uint8_t> response);

    // Quick Event Log handler (0x0146)
    int handleQuickEventLog(const ChifPktHeader& reqHdr,
                            std::span<const uint8_t> reqPayload,
                            std::span<uint8_t> response);

    // Sprint 2 handlers
    int handleSecurityStateGet(const ChifPktHeader& reqHdr,
                               std::span<uint8_t> response);
    int handleSecurityStateSet(const ChifPktHeader& reqHdr,
                               std::span<const uint8_t> reqPayload,
                               std::span<uint8_t> response);
    int handlePostState(const ChifPktHeader& reqHdr,
                        std::span<const uint8_t> reqPayload,
                        std::span<uint8_t> response);
    int handleLicenseKey(const ChifPktHeader& reqHdr,
                         std::span<uint8_t> response);
    int handleFieldAccess(const ChifPktHeader& reqHdr,
                          std::span<const uint8_t> reqPayload,
                          std::span<uint8_t> response);
    int handleBootProgress(const ChifPktHeader& reqHdr,
                           std::span<const uint8_t> reqPayload,
                           std::span<uint8_t> response);
    int handleBiosSync(const ChifPktHeader& reqHdr,
                       std::span<const uint8_t> reqPayload,
                       std::span<uint8_t> response);

    // Sprint 4 handlers
    int handleGpioCpld(const ChifPktHeader& reqHdr,
                       std::span<const uint8_t> reqPayload,
                       std::span<uint8_t> response);
    int handlePlatDef(const ChifPktHeader& reqHdr,
                      std::span<const uint8_t> reqPayload,
                      std::span<uint8_t> response);
    int handlePlatDefV2(const ChifPktHeader& reqHdr,
                        std::span<const uint8_t> reqPayload,
                        std::span<uint8_t> response);
    int handlePowerRegulator(const ChifPktHeader& reqHdr,
                             std::span<const uint8_t> reqPayload,
                             std::span<uint8_t> response);

    // PlatDef v2 blob storage — BIOS uploads platform topology data
    // via 0x0203-0x0204, then reads it back via 0x0205-0x0206.
    std::vector<uint8_t> platDefBlob_;

    // Sprint 3 handlers
    int handleIPv4Config(const ChifPktHeader& reqHdr,
                         std::span<uint8_t> response);
    int handleNicConfig(const ChifPktHeader& reqHdr,
                        std::span<uint8_t> response);
    int handlePciDeviceInfo(const ChifPktHeader& reqHdr,
                            std::span<const uint8_t> reqPayload,
                            std::span<uint8_t> response);
    int handleIPv6Config(const ChifPktHeader& reqHdr,
                         std::span<uint8_t> response);

    // Shared helper for IPv4/NIC config responses (I2: dedup)
    int fillNetworkResponse(const ChifPktHeader& reqHdr,
                            std::span<uint8_t> response);

    // I2C proxy handler (0x0072)
    int handleI2cTransaction(const ChifPktHeader& reqHdr,
                             std::span<const uint8_t> reqPayload,
                             std::span<uint8_t> response);

    // Platform info command handlers
    int handleHwRevision(const ChifPktHeader& reqHdr,
                         std::span<uint8_t> response);

    int handleFlashInfo(const ChifPktHeader& reqHdr,
                        std::span<uint8_t> response);

    int handleDateTime(const ChifPktHeader& reqHdr,
                       std::span<uint8_t> response);

    // Build a minimal response with just ErrorCode.
    static int buildSimpleResponse(const ChifPktHeader& reqHdr,
                                   std::span<uint8_t> response,
                                   uint32_t errorCode);

    // Build a response with name + data (used by 0x012B and 0x0130).
    static int buildEvDataResponse(const ChifPktHeader& reqHdr,
                                   std::span<uint8_t> response,
                                   const EvEntry& ev);
};

// Minimal Health service handler (SVC=0x10) — BIOS checks BMC health.
// Responds with success (ErrorCode=0) to all commands.
class HealthService : public ServiceHandler
{
  public:
    int handle(std::span<const uint8_t> request,
               std::span<uint8_t> response) override;

    uint8_t serviceId() const override
    {
        return healthServiceId;
    }
};

} // namespace chif
