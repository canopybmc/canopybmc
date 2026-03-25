// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH

#include "smif_service.hpp"

#include <phosphor-logging/lg2.hpp>

#include <cstring>

namespace chif
{

int SmifService::handlePlatDef(const ChifPktHeader& reqHdr,
                               std::span<const uint8_t> reqPayload,
                               std::span<uint8_t> response)
{
    // Platform Definition (0x0200/0x0202): Response must be full pkt_8200
    // size (8 + 4024 = 4032 bytes). The BIOS validates response size.
    //
    // pkt_8200 layout:
    //   ErrorCode(4) + op(2) + flags(2) + data_size(4) + data_offset(4)
    //   + timestamp(4) + recordID(2) + count(2) + data[4000]
    constexpr uint16_t pkt8200Size = 4024;
    constexpr uint16_t respSize =
        static_cast<uint16_t>(sizeof(ChifPktHeader) + pkt8200Size);

    if (response.size() < respSize)
    {
        return buildSimpleResponse(reqHdr, response, 0);
    }

    initResponse(response, reqHdr, respSize);
    auto resp = responsePayload(response);
    std::memset(resp.data(), 0, pkt8200Size);

    // Parse request pkt_0200 fields
    uint16_t op = 0;
    uint32_t timestamp = 0;
    uint16_t recordID = 0;

    if (reqPayload.size() >= 6)
    {
        std::memcpy(&op, reqPayload.data() + 4, sizeof(op));
    }
    if (reqPayload.size() >= 16)
    {
        std::memcpy(&timestamp, reqPayload.data() + 12, sizeof(timestamp));
    }
    if (reqPayload.size() >= 18)
    {
        std::memcpy(&recordID, reqPayload.data() + 16, sizeof(recordID));
    }

    uint32_t errCode = 0;
    uint16_t respCount = 0;
    uint32_t dataSize = 0;

    switch (op)
    {
        case 11: // PLATDEF_CMD_DOWNLD_SPEC_PLATDEF_DATA_BY_TYPE
        {
            if (recordID == 5) // Power Supply type
            {
                // Return record IDs for PSU1 (0x47) and PSU2 (0x48)
                uint8_t ids[] = {0x47, 0x00, 0x00, 0x00,
                                 0x48, 0x00, 0x00, 0x00};
                std::memcpy(resp.data() + 24, ids, sizeof(ids));
                respCount = 2;
                dataSize = sizeof(ids);
            }
            else
            {
                // No records for other types
                respCount = 0;
                dataSize = 0;
            }
            break;
        }
        case 9: // PLATDEF_CMD_DOWNLD_SPEC_PLATDEF_DATA
        {
            // Return specific record data by ID.
            // Request data[0..1] = recordID to fetch.
            uint16_t reqRecId = 0;
            if (reqPayload.size() >= 26)
            {
                std::memcpy(&reqRecId, reqPayload.data() + 24, sizeof(reqRecId));
            }

            if (reqRecId == 0x0047) // PSU1
            {
                // 6 bytes: RecordID(2) + reserved(2) + segment(1) + addr(1)
                uint8_t d[] = {0x47, 0x00, 0x43, 0x00, 0x66, 0xB0};
                std::memcpy(resp.data() + 24, d, sizeof(d));
                respCount = 1;
                dataSize = sizeof(d);
            }
            else if (reqRecId == 0x0048) // PSU2
            {
                uint8_t d[] = {0x48, 0x00, 0x43, 0x00, 0x67, 0xB2};
                std::memcpy(resp.data() + 24, d, sizeof(d));
                respCount = 1;
                dataSize = sizeof(d);
            }
            else
            {
                errCode = 0xFFFF;
            }
            break;
        }
        default:
            // Return empty data for unsupported sub-commands.
            // ErrorCode=0 with data_size=0 means "no data available."
            break;
    }

    // Fill pkt_8200 fixed header
    std::memcpy(resp.data() + 0, &errCode, sizeof(errCode));
    std::memcpy(resp.data() + 4, &op, sizeof(op));
    std::memcpy(resp.data() + 8, &dataSize, sizeof(dataSize));
    std::memcpy(resp.data() + 16, &timestamp, sizeof(timestamp));
    std::memcpy(resp.data() + 20, &recordID, sizeof(recordID));
    std::memcpy(resp.data() + 22, &respCount, sizeof(respCount));

    return respSize;
}

int SmifService::handlePlatDefV2(const ChifPktHeader& reqHdr,
                                 std::span<const uint8_t> reqPayload,
                                 std::span<uint8_t> response)
{
    // PlatDef v2 protocol (0x0203-0x0207):
    //   0x0203 = Begin upload — clear blob, ACK
    //   0x0204 = Upload chunk — append payload to blob
    //   0x0205 = Query — BIOS asks for specific records (like PlatDef download)
    //   0x0206 = Download — BIOS reads back PlatDef data
    //   0x0207 = End upload — finalize blob, ACK
    //
    // The BIOS first uploads the entire platform definition (~300KB),
    // then reads specific records back (PSU I2C info, Megacell info, etc.)
    //
    // Response format: same as pkt_8200 (24-byte header + data)

    switch (reqHdr.command)
    {
        case smifCmdPlatDefV2Begin: // 0x0203
        {
            platDefBlob_.clear();
            platDefBlob_.reserve(400000); // ~300KB typical
            lg2::info("PlatDef v2: begin upload, blob cleared");
            return buildSimpleResponse(reqHdr, response, 0);
        }

        case smifCmdPlatDefV2Chunk: // 0x0204
        {
            // Append payload data to blob (skip the header portion)
            // The payload contains: offset(4) + data_size(4) + data[]
            if (reqPayload.size() >= 8)
            {
                uint32_t offset = 0;
                uint32_t dataSize = 0;
                std::memcpy(&offset, reqPayload.data(), sizeof(offset));
                std::memcpy(&dataSize, reqPayload.data() + 4, sizeof(dataSize));

                // Overflow and size limit check
                constexpr size_t maxPlatDefSize = 512 * 1024;
                size_t endOffset =
                    static_cast<size_t>(offset) + dataSize;
                if (endOffset < static_cast<size_t>(offset) ||
                    endOffset > maxPlatDefSize)
                {
                    return buildSimpleResponse(reqHdr, response, 1);
                }

                if (dataSize > 0 && reqPayload.size() >= 8 &&
                    (reqPayload.size() - 8) >= dataSize)
                {
                    // Ensure blob is large enough
                    if (endOffset > platDefBlob_.size())
                    {
                        platDefBlob_.resize(endOffset, 0);
                    }
                    std::memcpy(platDefBlob_.data() + offset,
                                reqPayload.data() + 8, dataSize);
                }
            }
            return buildSimpleResponse(reqHdr, response, 0);
        }

        case smifCmdPlatDefV2End: // 0x0207
        {
            lg2::info("PlatDef v2: upload complete, blob={SZ} bytes",
                      "SZ", static_cast<uint32_t>(platDefBlob_.size()));
            // Free the upload buffer — data is not needed after upload
            platDefBlob_ = {};
            return buildSimpleResponse(reqHdr, response, 0);
        }

        case smifCmdPlatDefV2Query: // 0x0205
        case smifCmdPlatDefV2Download: // 0x0206
        {
            // Mirror the request size with ErrorCode=0, just like
            // SmifPkt_not_implemented. The BIOS validates response size.
            uint16_t mirrorSize = reqHdr.pktSize;
            if (mirrorSize < sizeof(ChifPktHeader) + sizeof(uint32_t))
            {
                mirrorSize = static_cast<uint16_t>(
                    sizeof(ChifPktHeader) + sizeof(uint32_t));
            }
            if (response.size() >= mirrorSize)
            {
                initResponse(response, reqHdr, mirrorSize);
                auto resp = responsePayload(response);
                std::memset(resp.data(), 0,
                            mirrorSize - sizeof(ChifPktHeader));
                uint32_t ec = 0;
                std::memcpy(resp.data(), &ec, sizeof(ec));
                return mirrorSize;
            }
            // fallback: return simple response
            return buildSimpleResponse(reqHdr, response, 0);
        }
    }

    return buildSimpleResponse(reqHdr, response, 0);
}

} // namespace chif
