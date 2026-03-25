// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH

#include "smif_service.hpp"
#include "platform_info.hpp"

#include <phosphor-logging/lg2.hpp>

#include <cstring>

namespace chif
{

SmifService::SmifService(EvStorage& evStorage, EventLogger* eventLogger,
                         const std::string& osReleasePath,
                         const std::string& serialNumberPath) :
    evStorage_(evStorage), eventLogger_(eventLogger)
{
    // Cache platform info at construction — these files don't change at runtime
    auto versionOpt = readOsReleaseField("VERSION_ID", osReleasePath);
    versionId_ = versionOpt.value_or("0.0.0");

    auto buildOpt = readOsReleaseField("BUILD_ID", osReleasePath);
    buildId_ = buildOpt.value_or("unknown");

    boardSerial_ = readDeviceTreeString(serialNumberPath);

    // Read product ID from device tree; fall back to hardcoded default
    productId_ = readDeviceTreeString("/proc/device-tree/model");
    if (productId_.empty())
    {
        productId_ = "P71673-425"; // DL360 Gen11 fallback
    }

    fwVersionPacked_ = parseFirmwareVersion(versionId_);

    lg2::info(
        "SMIF: platform info cached — version={VER} build={BUILD} serial={SER} prodId={PID}",
        "VER", versionId_, "BUILD", buildId_, "SER", boardSerial_, "PID",
        productId_);
}

int SmifService::handle(std::span<const uint8_t> request,
                        std::span<uint8_t> response)
{
    if (request.size() < sizeof(ChifPktHeader))
    {
        return -1;
    }

    auto hdr = parseHeader(request);
    auto reqPayload = payload(request);

    switch (hdr.command)
    {
        // --- I2C proxy ---
        case smifCmdI2cTransaction:
            return handleI2cTransaction(hdr, reqPayload, response);

        // --- Sprint 2: Core BIOS integration ---
        case smifCmdSecurityStateGet:
            return handleSecurityStateGet(hdr, response);

        case smifCmdSecurityStateSet:
            return handleSecurityStateSet(hdr, reqPayload, response);

        case smifCmdPostState:
            return handlePostState(hdr, reqPayload, response);

        case smifCmdLicenseKey:
            return handleLicenseKey(hdr, response);

        case smifCmdFieldAccess:
            return handleFieldAccess(hdr, reqPayload, response);

        case smifCmdBootProgress:
            return handleBootProgress(hdr, reqPayload, response);

        case smifCmdBiosSync:
            return handleBiosSync(hdr, reqPayload, response);

        // --- Sprint 3: Network & Hardware ---
        case smifCmdIPv4Config:
            return handleIPv4Config(hdr, response);

        case smifCmdNicConfig:
            return handleNicConfig(hdr, response);

        case smifCmdPciDeviceInfo:
            return handlePciDeviceInfo(hdr, reqPayload, response);

        case smifCmdIPv6Config:
            return handleIPv6Config(hdr, response);

        // --- Sprint 4: Hardware access ---
        case smifCmdGpioCpld:
            return handleGpioCpld(hdr, reqPayload, response);

        case smifCmdPlatDefUpload:
        case smifCmdPlatDefDownload:
            return handlePlatDef(hdr, reqPayload, response);

        case smifCmdPlatDefV2Begin:
        case smifCmdPlatDefV2Chunk:
        case smifCmdPlatDefV2Query:
        case smifCmdPlatDefV2Download:
        case smifCmdPlatDefV2End:
            return handlePlatDefV2(hdr, reqPayload, response);

        case smifCmdPowerRegulator:
            return handlePowerRegulator(hdr, reqPayload, response);

        // --- Platform info commands ---
        case smifCmdHwRevision:
            return handleHwRevision(hdr, response);

        case smifCmdFlashInfo:
            return handleFlashInfo(hdr, response);

        case smifCmdDateTime:
            return handleDateTime(hdr, response);

        // --- EV commands ---
        case smifCmdGetEvByIndex:
            return handleGetEvByIndex(hdr, reqPayload, response);

        case smifCmdSetDeleteEv:
            return handleSetDeleteEv(hdr, reqPayload, response);

        case smifCmdGetEvByName:
            return handleGetEvByName(hdr, reqPayload, response);

        case smifCmdEvStats:
            return handleEvStats(hdr, response);

        case smifCmdEvState:
            return handleEvState(hdr, response);

        // --- Commands that MUST return error to avoid infinite loops ---
        case smifCmdGetEvAuthStatus:
            // BIOS image auth status — return "not implemented"
            // This prevents the BIOS polling loop (see spec §5.10)
            return buildSimpleResponse(hdr, response, 1);

        // --- Event logging ---
        case smifCmdQuickEventLog:
            return handleQuickEventLog(hdr, reqPayload, response);

        // --- Commands that return success ---
        case smifCmdBiosReady:
            lg2::info("SMIF: BIOS readiness query — responding ready");
            return buildSimpleResponse(hdr, response, 0);

        default:
        {
            // Mirror the request size in the response with ErrorCode=0.
            // HPE's SmifPkt_not_implemented() does this — the BIOS validates
            // response size and rejects short responses as "CHIF response
            // invalid". By matching the request size, we pass the
            // validation and the BIOS treats it as a successful no-op.
            uint16_t mirrorSize = hdr.pktSize;
            if (mirrorSize < sizeof(ChifPktHeader) + sizeof(uint32_t))
            {
                mirrorSize = static_cast<uint16_t>(sizeof(ChifPktHeader) +
                                                   sizeof(uint32_t));
            }
            if (response.size() < mirrorSize)
            {
                // Response buffer too small — fall back to minimum
                return buildSimpleResponse(hdr, response, 0);
            }
            initResponse(response, hdr, mirrorSize);
            auto resp = responsePayload(response);
            size_t payloadLen =
                static_cast<size_t>(mirrorSize) - sizeof(ChifPktHeader);
            if (payloadLen > resp.size())
            {
                return buildSimpleResponse(hdr, response, 0);
            }
            std::memset(resp.data(), 0, payloadLen);
            // ErrorCode = 0 at offset 0
            uint32_t errorCode = 0;
            std::memcpy(resp.data(), &errorCode, sizeof(errorCode));
            return mirrorSize;
        }
    }
}

int SmifService::handleQuickEventLog(const ChifPktHeader& reqHdr,
                                     std::span<const uint8_t> reqPayload,
                                     std::span<uint8_t> response)
{
    // Minimum payload: evtType(1) + matchCode(2) + evtClass(2) +
    //                  evtCode(2) + severity(1) + evtVarLen(2) = 10 bytes
    if (reqPayload.size() < 10)
    {
        return buildSimpleResponse(reqHdr, response, 2);
    }

    uint8_t evtType = reqPayload[0];

    uint16_t matchCode = 0;
    std::memcpy(&matchCode, reqPayload.data() + 1, sizeof(matchCode));

    uint16_t evtClass = 0;
    std::memcpy(&evtClass, reqPayload.data() + 3, sizeof(evtClass));

    uint16_t evtCode = 0;
    std::memcpy(&evtCode, reqPayload.data() + 5, sizeof(evtCode));

    uint8_t severity = reqPayload[7];

    uint16_t evtVarLen = 0;
    std::memcpy(&evtVarLen, reqPayload.data() + 8, sizeof(evtVarLen));

    // Validate variable data length against actual remaining payload
    auto varData = reqPayload.subspan(10);
    if (varData.size() < evtVarLen)
    {
        evtVarLen = static_cast<uint16_t>(varData.size());
    }

    uint32_t evtNum = 0;
    if (eventLogger_)
    {
        evtNum = eventLogger_->log(evtType, evtClass, evtCode, severity,
                                   matchCode, varData.first(evtVarLen));
    }

    // Response: errorCode(1) + evtType(1) + evtNum(4) = 6 bytes
    constexpr uint16_t payloadSize = 6;
    constexpr uint16_t respSize =
        static_cast<uint16_t>(sizeof(ChifPktHeader) + payloadSize);

    if (response.size() < respSize)
    {
        return -1;
    }

    initResponse(response, reqHdr, respSize);
    auto resp = responsePayload(response);

    resp[0] = 0; // errorCode (uint8_t)
    resp[1] = evtType;
    std::memcpy(resp.data() + 2, &evtNum, sizeof(evtNum));

    return respSize;
}

int SmifService::buildSimpleResponse(const ChifPktHeader& reqHdr,
                                     std::span<uint8_t> response,
                                     uint32_t errorCode)
{
    constexpr uint16_t respSize =
        static_cast<uint16_t>(sizeof(ChifPktHeader) + sizeof(uint32_t));

    if (response.size() < respSize)
    {
        return -1;
    }

    initResponse(response, reqHdr, respSize);
    auto resp = responsePayload(response);
    std::memcpy(resp.data(), &errorCode, sizeof(errorCode));

    return respSize;
}

// --- HealthService ---

int HealthService::handle(std::span<const uint8_t> request,
                          std::span<uint8_t> response)
{
    if (request.size() < sizeof(ChifPktHeader))
    {
        return -1;
    }

    auto hdr = parseHeader(request);

    // APML Version query (0x001C): BIOS asks for APML version info.
    // Return a properly sized response to avoid "CHIF response invalid".
    if (hdr.command == healthCmdApmlVersion)
    {
        // Return a 12-byte response (header + ErrorCode=0)
        // The BIOS accepts this as "APML available, version 0"
        constexpr uint16_t apmlRespSize =
            static_cast<uint16_t>(sizeof(ChifPktHeader) + sizeof(uint32_t));
        if (response.size() < apmlRespSize)
        {
            return -1;
        }
        initResponse(response, hdr, apmlRespSize);
        auto resp = responsePayload(response);
        uint32_t errCode = 0;
        std::memcpy(resp.data(), &errCode, sizeof(errCode));
        return apmlRespSize;
    }

    // Health LED control (0x0114): BIOS sets/gets system health LED.
    // Response must be 15 bytes (Size=0x0F) with LED state at offset +11.
    if (hdr.command == healthCmdLedControl)
    {
        constexpr uint16_t ledRespSize = 15;
        if (response.size() < ledRespSize)
        {
            return -1;
        }

        initResponse(response, hdr, ledRespSize);
        auto resp = responsePayload(response);
        // Payload: reserved(3) + currentState(1) + padding(3)
        std::memset(resp.data(), 0, ledRespSize - sizeof(ChifPktHeader));
        // CurrentState = 0 (LED off / healthy)
        resp[3] = 0;

        return ledRespSize;
    }

    // HPE's HealthHandler dispatches on the interface type (first payload
    // byte, which corresponds to our header command field). Only three
    // commands get a response via GenResponse():
    //   0x11 (NVRAM_SECURE), 0x1C (APML_VERSION), 0x1D (POST_FLAGS)
    //
    // All others — including 0x12 (RESET_STATUS), 0x14 (LED_CONTROL),
    // and 0x15 (ENCAPSULATED_IPMI) — return -1 (no response). The BIOS
    // expects a clean timeout for these, not a malformed response.
    // Returning a 12-byte response causes the BIOS to parse garbage.
    switch (hdr.command)
    {
        case healthCmdNvramSecure: // NVRAM_SECURE
        case healthCmdPostFlags:  // POST_FLAGS / Health Query
        {
            // Return ErrorCode=0. For 0x001D, this means HealthState=0
            // (healthy). A non-zero HealthState can HALT POST.
            constexpr uint16_t respSize = static_cast<uint16_t>(
                sizeof(ChifPktHeader) + sizeof(uint32_t));
            if (response.size() < respSize)
            {
                return -1;
            }
            initResponse(response, hdr, respSize);
            auto resp = responsePayload(response);
            uint32_t errorCode = 0;
            std::memcpy(resp.data(), &errorCode, sizeof(errorCode));
            return respSize;
        }
        default:
            // No response — let the BIOS time out cleanly.
            // This matches HPE's behavior for IPMI (0x15),
            // RESET_STATUS (0x12), and all other Health commands.
            return -1;
    }
}

} // namespace chif
