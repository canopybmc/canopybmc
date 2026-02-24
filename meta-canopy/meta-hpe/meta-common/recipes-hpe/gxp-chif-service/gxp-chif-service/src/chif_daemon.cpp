// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#include "chif_daemon.hpp"

#include <phosphor-logging/lg2.hpp>

namespace chif
{

ChifDaemon::ChifDaemon(std::unique_ptr<Channel> channel) :
    channel_(std::move(channel))
{}

void ChifDaemon::registerHandler(std::unique_ptr<ServiceHandler> handler)
{
    uint8_t id = handler->serviceId();
    handlers_[id] = std::move(handler);
}

void ChifDaemon::run()
{
    running_ = true;
    lg2::info("CHIF daemon starting");

    while (running_)
    {
        auto n = channel_->read(recvBuf_);
        if (n <= 0)
        {
            continue;
        }

        if (static_cast<size_t>(n) < sizeof(ChifPktHeader))
        {
            lg2::warning("Short packet received: {SIZE} bytes", "SIZE", n);
            continue;
        }

        auto hdr = parseHeader(
            std::span<const uint8_t>(recvBuf_.data(), static_cast<size_t>(n)));

        // Copy packed fields to locals — packed fields cannot bind to
        // references, which lg2 requires.
        uint8_t svcId = hdr.serviceId;
        uint16_t cmd = hdr.command;
        uint16_t seq = hdr.sequence;

        lg2::info(
            "CHIF RX: svc=0x{SVC:02x} cmd=0x{CMD:04x} seq={SEQ} size={SZ}",
            "SVC", svcId, "CMD", cmd, "SEQ", seq, "SZ", n);

        auto it = handlers_.find(hdr.serviceId);
        if (it == handlers_.end())
        {
            lg2::warning(
                "CHIF DROP: no handler for svc=0x{SVC:02x} cmd=0x{CMD:04x}",
                "SVC", svcId, "CMD", cmd);
            continue;
        }

        auto reqSpan = std::span<const uint8_t>(recvBuf_.data(),
                                                static_cast<size_t>(n));
        auto respSpan = std::span<uint8_t>(respBuf_);

        int respSize = it->second->handle(reqSpan, respSpan);
        if (respSize > 0)
        {
            auto rspHdr = parseHeader(
                std::span<const uint8_t>(respBuf_.data(),
                                         static_cast<size_t>(respSize)));
            uint8_t rspSvc = rspHdr.serviceId;
            uint16_t rspCmd = rspHdr.command;
            uint16_t rspSeq = rspHdr.sequence;
            lg2::info(
                "CHIF TX: svc=0x{SVC:02x} cmd=0x{CMD:04x} seq={SEQ} size={SZ}",
                "SVC", rspSvc, "CMD", rspCmd, "SEQ", rspSeq, "SZ", respSize);
            channel_->write(
                std::span<const uint8_t>(respBuf_.data(),
                                         static_cast<size_t>(respSize)));
        }
        else
        {
            lg2::info("CHIF: handler returned no response for cmd=0x{CMD:04x}",
                      "CMD", cmd);
        }
    }

    lg2::info("CHIF daemon stopped");
}

void ChifDaemon::stop()
{
    running_ = false;
}

} // namespace chif
