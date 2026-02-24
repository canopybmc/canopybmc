// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#pragma once

#include "../src/packet.hpp"

#include <cstring>
#include <deque>
#include <vector>

namespace chif::test
{

// In-memory channel for unit testing. Packets are queued for reading,
// and written packets are captured for inspection.
class MockChannel : public Channel
{
  public:
    // Queue a packet for the daemon to read.
    void enqueuePacket(std::span<const uint8_t> pkt)
    {
        pendingReads_.emplace_back(pkt.begin(), pkt.end());
    }

    // Build and enqueue a CHIF packet with the given header fields and payload.
    void enqueuePacket(uint16_t command, uint8_t serviceId,
                       uint16_t sequence, std::span<const uint8_t> payload)
    {
        std::vector<uint8_t> pkt(sizeof(ChifPktHeader) + payload.size());
        ChifPktHeader hdr{};
        hdr.pktSize = static_cast<uint16_t>(pkt.size());
        hdr.sequence = sequence;
        hdr.command = command;
        hdr.serviceId = serviceId;
        hdr.version = 0x01;
        std::memcpy(pkt.data(), &hdr, sizeof(hdr));
        if (!payload.empty())
        {
            std::memcpy(pkt.data() + sizeof(hdr), payload.data(),
                        payload.size());
        }
        pendingReads_.push_back(std::move(pkt));
    }

    // Convenience: enqueue a header-only packet (no payload).
    void enqueuePacket(uint16_t command, uint8_t serviceId,
                       uint16_t sequence = 1)
    {
        enqueuePacket(command, serviceId, sequence,
                      std::span<const uint8_t>{});
    }

    ssize_t read(std::span<uint8_t> buf) override
    {
        if (pendingReads_.empty())
        {
            return 0; // signal end-of-data
        }
        auto& pkt = pendingReads_.front();
        size_t n = std::min(buf.size(), pkt.size());
        std::memcpy(buf.data(), pkt.data(), n);
        pendingReads_.pop_front();
        return static_cast<ssize_t>(n);
    }

    ssize_t write(std::span<const uint8_t> buf) override
    {
        writtenPackets_.emplace_back(buf.begin(), buf.end());
        return static_cast<ssize_t>(buf.size());
    }

    // Access captured responses.
    const std::vector<std::vector<uint8_t>>& writtenPackets() const
    {
        return writtenPackets_;
    }

    // Get the last written response header (convenience).
    ChifPktHeader lastResponseHeader() const
    {
        if (writtenPackets_.empty())
        {
            return {};
        }
        return parseHeader(writtenPackets_.back());
    }

    // Get the last written response payload (after header).
    std::vector<uint8_t> lastResponsePayload() const
    {
        if (writtenPackets_.empty())
        {
            return {};
        }
        auto& pkt = writtenPackets_.back();
        if (pkt.size() <= sizeof(ChifPktHeader))
        {
            return {};
        }
        return {pkt.begin() + sizeof(ChifPktHeader), pkt.end()};
    }

    void clearWritten()
    {
        writtenPackets_.clear();
    }

  private:
    std::deque<std::vector<uint8_t>> pendingReads_;
    std::vector<std::vector<uint8_t>> writtenPackets_;
};

} // namespace chif::test
