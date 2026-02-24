// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#include "rom_service.hpp"

#include <phosphor-logging/lg2.hpp>

#include <cstring>

namespace chif
{

RomService::RomService(SmbiosWriter& writer, MdrBridge* mdrBridge) :
    writer_(writer), mdrBridge_(mdrBridge)
{}

int RomService::handle(std::span<const uint8_t> request,
                       std::span<uint8_t> response)
{
    if (request.size() < sizeof(ChifPktHeader))
    {
        return -1;
    }

    auto hdr = parseHeader(request);

    switch (hdr.command)
    {
        case romCmdBegin:
            return handleBegin(hdr, response);
        case romCmdRecord:
            return handleRecord(hdr, request, response);
        case romCmdEnd:
            return handleEnd(hdr, response);
        case romCmdPowerData:
            lg2::info("ROM: power data received (ack only)");
            return buildHeaderOnlyResponse(hdr, response);
        case romCmdThermal:
            lg2::info("ROM: thermal data received (ack only)");
            return buildHeaderOnlyResponse(hdr, response);
        case romCmdBlob:
            return handleBlob(hdr, request, response);
        default:
            lg2::warning("ROM: unknown command 0x{CMD:04x}, dropping",
                         "CMD", static_cast<uint16_t>(hdr.command));
            return -1;
    }
}

int RomService::handleBegin(const ChifPktHeader& reqHdr,
                            std::span<uint8_t> response)
{
    lg2::info("ROM: begin SMBIOS reception");
    writer_.begin();
    // HPE reference: header-only response (8 bytes), no payload, no response bit
    return buildHeaderOnlyResponse(reqHdr, response);
}

int RomService::handleRecord(const ChifPktHeader& reqHdr,
                             std::span<const uint8_t> request,
                             std::span<uint8_t> response)
{
    auto data = payload(request);
    if (data.empty())
    {
        lg2::warning("ROM: empty record received");
        return buildHeaderOnlyResponse(reqHdr, response);
    }

    writer_.addRecord(data);
    // HPE framework: default 8-byte header response for all ROM commands
    return buildHeaderOnlyResponse(reqHdr, response);
}

int RomService::handleEnd(const ChifPktHeader& reqHdr,
                          std::span<uint8_t> response)
{
    lg2::info("ROM: end SMBIOS reception, {SIZE} bytes accumulated", "SIZE",
              writer_.dataSize());

    bool ok = writer_.finalize();
    if (!ok)
    {
        lg2::error("ROM: SMBIOS finalization failed");
    }

    // HPE reference: send header-only response first, then finalize
    int respSize = buildHeaderOnlyResponse(reqHdr, response);

    // Trigger smbios-mdr to parse the new data
    if (ok && mdrBridge_)
    {
        mdrBridge_->synchronize();
    }

    return respSize;
}

int RomService::handleBlob(const ChifPktHeader& reqHdr,
                           std::span<const uint8_t> request,
                           std::span<uint8_t> response)
{
    // Blob command sends multiple SMBIOS records in a single packet.
    auto data = payload(request);
    if (data.empty())
    {
        lg2::warning("ROM: empty blob received");
        return buildHeaderOnlyResponse(reqHdr, response);
    }

    lg2::info("ROM: blob received, {SIZE} bytes", "SIZE", data.size());
    writer_.addRecord(data);
    // HPE framework: default 8-byte header response for all ROM commands
    return buildHeaderOnlyResponse(reqHdr, response);
}

int RomService::buildHeaderOnlyResponse(const ChifPktHeader& reqHdr,
                                        std::span<uint8_t> response)
{
    // ROM responses: 8-byte header only, no payload, no response bit.
    // This matches the HPE reference romchf_submit() behavior.
    constexpr uint16_t respSize = static_cast<uint16_t>(sizeof(ChifPktHeader));

    if (response.size() < respSize)
    {
        return -1;
    }

    initRomResponse(response, reqHdr, respSize);
    return respSize;
}

} // namespace chif
