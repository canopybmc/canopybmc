// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#pragma once

#include <cstdint>
#include <cstring>
#include <span>
#include <sys/types.h>

namespace chif
{

// CHIF packet header — 8 bytes, little-endian, packed.
// All multi-byte fields are stored in host byte order (LE on GXP).
struct ChifPktHeader
{
    uint16_t pktSize;
    uint16_t sequence;
    uint16_t command;
    uint8_t serviceId;
    uint8_t version;
} __attribute__((packed));

static_assert(sizeof(ChifPktHeader) == 8, "ChifPktHeader must be 8 bytes");

// Maximum CHIF packet size (kernel driver limit)
inline constexpr size_t maxPacketSize = 4096;

// Maximum payload size (packet minus header)
inline constexpr size_t maxPayloadSize = maxPacketSize - sizeof(ChifPktHeader);

// Response command bit — responses have bit 15 set
inline constexpr uint16_t responseBit = 0x8000;

// Service IDs
inline constexpr uint8_t smifServiceId = 0x00;
inline constexpr uint8_t romServiceId = 0x02;
inline constexpr uint8_t healthServiceId = 0x10;

// Parse a CHIF header from raw bytes. Caller must ensure buf.size() >= 8.
inline ChifPktHeader parseHeader(std::span<const uint8_t> buf)
{
    ChifPktHeader hdr{};
    std::memcpy(&hdr, buf.data(), sizeof(hdr));
    return hdr;
}

// Get a pointer to the payload portion of a packet buffer.
inline std::span<const uint8_t> payload(std::span<const uint8_t> pkt)
{
    if (pkt.size() <= sizeof(ChifPktHeader))
    {
        return {};
    }
    return pkt.subspan(sizeof(ChifPktHeader));
}

// Initialize a response header by echoing sequence, setting cmd|0x8000,
// and copying service_id and version from the request.
inline void initResponse(std::span<uint8_t> resp,
                         const ChifPktHeader& reqHdr, uint16_t respSize)
{
    ChifPktHeader rspHdr{};
    rspHdr.pktSize = respSize;
    rspHdr.sequence = reqHdr.sequence;
    rspHdr.command = reqHdr.command | responseBit;
    rspHdr.serviceId = reqHdr.serviceId;
    rspHdr.version = reqHdr.version;
    std::memcpy(resp.data(), &rspHdr, sizeof(rspHdr));
}

// Initialize a ROM response header — ROM service does NOT set the response
// bit (0x8000). The HPE reference echoes the command value unchanged.
inline void initRomResponse(std::span<uint8_t> resp,
                            const ChifPktHeader& reqHdr, uint16_t respSize)
{
    ChifPktHeader rspHdr{};
    rspHdr.pktSize = respSize;
    rspHdr.sequence = reqHdr.sequence;
    rspHdr.command = reqHdr.command; // NO response bit for ROM
    rspHdr.serviceId = reqHdr.serviceId;
    rspHdr.version = reqHdr.version;
    std::memcpy(resp.data(), &rspHdr, sizeof(rspHdr));
}

// Get mutable span to response payload (after header).
inline std::span<uint8_t> responsePayload(std::span<uint8_t> resp)
{
    if (resp.size() <= sizeof(ChifPktHeader))
    {
        return {};
    }
    return resp.subspan(sizeof(ChifPktHeader));
}

// Abstract channel for reading/writing CHIF packets.
// DeviceChannel wraps /dev/chif24; MockChannel is for testing.
class Channel
{
  public:
    virtual ~Channel() = default;
    virtual ssize_t read(std::span<uint8_t> buf) = 0;
    virtual ssize_t write(std::span<const uint8_t> buf) = 0;
};

// Service handler interface — one per service_id.
class ServiceHandler
{
  public:
    virtual ~ServiceHandler() = default;

    // Process a request packet and produce a response.
    // Returns the total response size (including header), or -1 for
    // "no response" (silently drop).
    virtual int handle(std::span<const uint8_t> request,
                       std::span<uint8_t> response) = 0;

    virtual uint8_t serviceId() const = 0;
};

} // namespace chif
