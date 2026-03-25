// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH

#include "smif_service.hpp"
#include "platform_info.hpp"

#include <phosphor-logging/lg2.hpp>

#include <cstring>
#include <ctime>

namespace chif
{

int SmifService::handleHwRevision(const ChifPktHeader& reqHdr,
                                  std::span<uint8_t> response)
{
    // Response payload: 95 bytes (0x5F) — see pkt_8002 struct in spec.
    // Reports BMC hardware revision, firmware version, serial number, and
    // current timestamp to the BIOS during POST.
    constexpr uint16_t payloadSize = 95;
    constexpr uint16_t respSize =
        static_cast<uint16_t>(sizeof(ChifPktHeader) + payloadSize);

    if (response.size() < respSize)
    {
        return -1;
    }

    initResponse(response, reqHdr, respSize);
    auto resp = responsePayload(response);

    // Zero-fill the entire payload first
    std::memset(resp.data(), 0, payloadSize);

    // ErrorCode (offset 0x00): 0 = success
    uint32_t errorCode = 0;
    std::memcpy(resp.data() + 0x00, &errorCode, sizeof(errorCode));

    // status_word (offset 0x04): 0
    // max_users (offset 0x06): 12
    uint16_t maxUsers = 12;
    std::memcpy(resp.data() + 0x06, &maxUsers, sizeof(maxUsers));

    // firmware_version (offset 0x08): packed major.minor
    std::memcpy(resp.data() + 0x08, &fwVersionPacked_,
                sizeof(fwVersionPacked_));

    // firmware_date (offset 0x0A): 0 (not packed)
    // post_errcode (offset 0x0E): 0

    // datetime (offset 0x12): current unix timestamp
    uint32_t now = static_cast<uint32_t>(std::time(nullptr));
    std::memcpy(resp.data() + 0x12, &now, sizeof(now));

    // hw_revision (offset 0x16): 0x06 = GXP
    uint32_t hwRevision = 0x06;
    std::memcpy(resp.data() + 0x16, &hwRevision, sizeof(hwRevision));

    // board_serial (offset 0x1A): 20 bytes, null-padded
    constexpr size_t serialFieldLen = 20;
    if (!boardSerial_.empty())
    {
        size_t copyLen = std::min(boardSerial_.size(), serialFieldLen - 1);
        std::memcpy(resp.data() + 0x1A, boardSerial_.c_str(), copyLen);
    }

    // cable_status (offset 0x2E): 0
    // latest_event_id (offset 0x30): 0
    // cfg_writecount (offset 0x34): EV write count
    uint32_t writeCount = evStorage_.count();
    std::memcpy(resp.data() + 0x34, &writeCount, sizeof(writeCount));

    // post_errcode_mask (offset 0x38): 0

    // class_ (offset 0x3C): 7
    resp[0x3C] = 7;

    // subclass (offset 0x3D): 0
    // rc_in_use (offset 0x3E): 0

    // application (offset 0x3F): 5
    resp[0x3F] = 5;

    // security_state (offset 0x40): 3 = PRODUCTION
    resp[0x40] = 3;

    // chip_id (offset 0x41): 0 (8 bytes)
    // patch_version (offset 0x49): 0 (4 bytes)
    // reserved (offset 0x4D): 0 (18 bytes)

    lg2::info("SMIF 0x0002: hardware revision response — "
              "hwRev=0x06 fwVer=0x{VER:04x} serial={SER}",
              "VER", fwVersionPacked_, "SER", boardSerial_);

    return respSize;
}

int SmifService::handleDateTime(const ChifPktHeader& reqHdr,
                                std::span<uint8_t> response)
{
    // Response payload: 12 bytes — ErrorCode + timestamp + tz + dst + pad.
    // BMC always runs in UTC, so tz_offset=0 and daylight=0.
    constexpr uint16_t payloadSize = 12;
    constexpr uint16_t respSize =
        static_cast<uint16_t>(sizeof(ChifPktHeader) + payloadSize);

    if (response.size() < respSize)
    {
        return -1;
    }

    initResponse(response, reqHdr, respSize);
    auto resp = responsePayload(response);

    // Zero-fill
    std::memset(resp.data(), 0, payloadSize);

    // ErrorCode (offset 0): 0
    uint32_t errorCode = 0;
    std::memcpy(resp.data() + 0, &errorCode, sizeof(errorCode));

    // datetime (offset 4): Unix timestamp
    uint32_t now = static_cast<uint32_t>(std::time(nullptr));
    std::memcpy(resp.data() + 4, &now, sizeof(now));

    // tz_offset (offset 8): 0 (UTC)
    int16_t tzOffset = 0;
    std::memcpy(resp.data() + 8, &tzOffset, sizeof(tzOffset));

    // daylight (offset 10): 0
    resp[10] = 0;

    // pad (offset 11): 0
    resp[11] = 0;

    lg2::debug("SMIF 0x0055: date/time response — timestamp={TS}", "TS", now);

    return respSize;
}

int SmifService::handleFlashInfo(const ChifPktHeader& reqHdr,
                                 std::span<uint8_t> response)
{
    // Response payload: 181 bytes (0xB5) — firmware identity, ASIC info,
    // flash state. Reports BMC firmware name/version/date to the BIOS.
    constexpr uint16_t payloadSize = 181;
    constexpr uint16_t respSize =
        static_cast<uint16_t>(sizeof(ChifPktHeader) + payloadSize);

    if (response.size() < respSize)
    {
        return -1;
    }

    initResponse(response, reqHdr, respSize);
    auto resp = responsePayload(response);

    // Zero-fill the entire payload
    std::memset(resp.data(), 0, payloadSize);

    // ErrorCode (offset 0x00): 0
    uint32_t errorCode = 0;
    std::memcpy(resp.data() + 0x00, &errorCode, sizeof(errorCode));

    // FlashSectorSize (offset 0x04): 64 KB
    uint32_t sectorSize = 0x10000;
    std::memcpy(resp.data() + 0x04, &sectorSize, sizeof(sectorSize));

    // FirmwareDate (offset 0x08): 20 bytes, null-padded
    // Parse BUILD_ID (e.g., "2026-03-19") or use buildId_ directly
    constexpr size_t dateFieldLen = 20;
    if (!buildId_.empty())
    {
        size_t copyLen = std::min(buildId_.size(), dateFieldLen - 1);
        std::memcpy(resp.data() + 0x08, buildId_.c_str(), copyLen);
    }

    // FirmwareTime (offset 0x1C): 20 bytes — use "00:00:00" as placeholder
    constexpr const char* fwTime = "00:00:00";
    std::memcpy(resp.data() + 0x1C, fwTime, std::strlen(fwTime));

    // FirmwareNumber (offset 0x30): 10 bytes — version string
    constexpr size_t fwNumFieldLen = 10;
    if (!versionId_.empty())
    {
        size_t copyLen = std::min(versionId_.size(), fwNumFieldLen - 1);
        std::memcpy(resp.data() + 0x30, versionId_.c_str(), copyLen);
    }

    // FirmwarePass (offset 0x3A): 10 bytes — "1+"
    constexpr const char* fwPass = "1+";
    std::memcpy(resp.data() + 0x3A, fwPass, std::strlen(fwPass));

    // FirmwareName (offset 0x44): 20 bytes — "Canopy OpenBMC"
    constexpr const char* fwName = "Canopy OpenBMC";
    std::memcpy(resp.data() + 0x44, fwName, std::strlen(fwName));

    // AsicMajor (offset 0x58): 0x06 (GXP)
    resp[0x58] = 0x06;

    // AsicMinor (offset 0x59): 0x02
    resp[0x59] = 0x02;

    // CPLDVersion (offset 0x5A): 0x09
    resp[0x5A] = 0x09;

    // HostCPLDVersion (offset 0x5B): 0

    // AsicRTL (offset 0x5C): 0x00060202
    uint32_t asicRtl = 0x00060202;
    std::memcpy(resp.data() + 0x5C, &asicRtl, sizeof(asicRtl));

    // Bootleg (offset 0x60): 80 bytes — "GXP OpenBMC"
    constexpr const char* bootleg = "GXP OpenBMC";
    std::memcpy(resp.data() + 0x60, bootleg, std::strlen(bootleg));

    // flashManufacturer (offset 0xB0): 0
    // flashDevice (offset 0xB1): 0
    // flashState (offset 0xB2): 0
    // flashStage (offset 0xB3): 0
    // flashPercent (offset 0xB4): 0

    lg2::info("SMIF 0x0050: flash info response — "
              "name=Canopy OpenBMC ver={VER} date={DATE}",
              "VER", versionId_, "DATE", buildId_);

    return respSize;
}

} // namespace chif
