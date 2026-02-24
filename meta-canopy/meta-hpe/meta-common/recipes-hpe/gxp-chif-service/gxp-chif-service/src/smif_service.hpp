// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#pragma once

#include "packet.hpp"

#include <phosphor-logging/lg2.hpp>

#include <cstring>

namespace chif
{

// SMIF service handler — responds to BIOS commands.
//
// Most commands return ErrorCode=1 (not implemented). Only the BIOS
// readiness command (0x0008) and boot progress acknowledgements return
// success, because the BIOS gates SMBIOS transfer on BMC readiness.
class SmifService : public ServiceHandler
{
  public:
    int handle(std::span<const uint8_t> request,
               std::span<uint8_t> response) override
    {
        if (request.size() < sizeof(ChifPktHeader))
        {
            return -1;
        }

        auto hdr = parseHeader(request);

        switch (hdr.command)
        {
            // --- Commands that MUST return error to avoid infinite loops ---
            case 0x012b: // EV get by index — must return "not found"
            case 0x012d: // EV get related — must return "not found"
            case 0x0130: // EV get by name — must return "not found"
                return buildSimpleResponse(hdr, response, 1);

            // --- Commands that return success ---
            case 0x0008: // BIOS readiness
                lg2::info("SMIF: BIOS readiness query — responding ready");
                return buildSimpleResponse(hdr, response, 0);
            default:
                // Accept all other commands with success.
                // This lets the BIOS proceed through its full POST flow
                // including UEFI var store uploads (0x0203-0x0207) and
                // other non-enumeration commands.
                return buildSimpleResponse(hdr, response, 0);
        }
    }

    uint8_t serviceId() const override
    {
        return smifServiceId;
    }

  private:
    static int buildSimpleResponse(const ChifPktHeader& reqHdr,
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
};

// Minimal Health service handler (SVC=0x10) — BIOS checks BMC health.
// Responds with success (ErrorCode=0) to all commands.
class HealthService : public ServiceHandler
{
  public:
    int handle(std::span<const uint8_t> request,
               std::span<uint8_t> response) override
    {
        if (request.size() < sizeof(ChifPktHeader))
        {
            return -1;
        }

        auto hdr = parseHeader(request);

        constexpr uint16_t respSize =
            static_cast<uint16_t>(sizeof(ChifPktHeader) + sizeof(uint32_t));

        if (response.size() < respSize)
        {
            return -1;
        }

        initResponse(response, hdr, respSize);

        uint32_t errorCode = 0;
        auto resp = responsePayload(response);
        std::memcpy(resp.data(), &errorCode, sizeof(errorCode));

        return respSize;
    }

    uint8_t serviceId() const override
    {
        return healthServiceId;
    }
};

} // namespace chif
