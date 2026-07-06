// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#include "chif_daemon.hpp"

#include <phosphor-logging/lg2.hpp>

#include <cerrno>
#include <cstring>

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

void ChifDaemon::run(sd_event* event)
{
    event_ = event;

    int fd = channel_->pollFd();
    if (fd < 0)
    {
        lg2::error("CHIF channel is not pollable; cannot run event loop");
        return;
    }

    int r = sd_event_add_io(event, &ioSource_, fd, EPOLLIN,
                            &ChifDaemon::onChannelIo, this);
    if (r < 0)
    {
        lg2::error("Failed to register CHIF io source: {ERR}", "ERR",
                   strerror(-r));
        return;
    }

    lg2::info("CHIF daemon starting");
    sd_event_loop(event);
    lg2::info("CHIF daemon stopped");

    sd_event_source_unref(ioSource_);
    ioSource_ = nullptr;
}

void ChifDaemon::stop()
{
    if (event_ != nullptr)
    {
        sd_event_exit(event_, 0);
    }
}

int ChifDaemon::onChannelIo(sd_event_source* /*source*/, int /*fd*/,
                            uint32_t /*revents*/, void* userdata)
{
    static_cast<ChifDaemon*>(userdata)->processOnce();
    return 0;
}

void ChifDaemon::processOnce()
{
    auto n = channel_->read(recvBuf_);
    if (n <= 0)
    {
        int savedErrno = errno;
        // ERINTR/EAGAIN expected on signal delivery or when no data was
        // received yet.
        if (n < 0 && savedErrno != EINTR && savedErrno != EAGAIN)
        {
            lg2::error("CHIF read failed: {ERR}", "ERR", strerror(savedErrno));
        }
        return;
    }

    if (static_cast<size_t>(n) < sizeof(ChifPktHeader))
    {
        lg2::warning("Short packet received: {SIZE} bytes", "SIZE", n);
        return;
    }

    auto hdr = parseHeader(
        std::span<const uint8_t>(recvBuf_.data(), static_cast<size_t>(n)));

    uint8_t svcId = hdr.serviceId;
    uint16_t cmd = hdr.command;
    uint16_t seq = hdr.sequence;

    lg2::debug("CHIF RX: svc={SVC} cmd={CMD} seq={SEQ} size={SZ}", "SVC",
               lg2::hex, svcId, "CMD", lg2::hex, cmd, "SEQ", seq, "SZ", n);

    auto it = handlers_.find(hdr.serviceId);
    if (it == handlers_.end())
    {
        lg2::warning("CHIF DROP: no handler for svc={SVC} cmd={CMD}", "SVC",
                     lg2::hex, svcId, "CMD", lg2::hex, cmd);
        return;
    }

    auto reqSpan =
        std::span<const uint8_t>(recvBuf_.data(), static_cast<size_t>(n));
    auto respSpan = std::span<uint8_t>(respBuf_);

    int respSize = it->second->handle(reqSpan, respSpan);
    if (respSize > 0)
    {
        if (static_cast<size_t>(respSize) < sizeof(ChifPktHeader))
        {
            lg2::error("Handler returned invalid response size: {SZ}", "SZ",
                       respSize);
            return;
        }

        auto rspHdr = parseHeader(std::span<const uint8_t>(
            respBuf_.data(), static_cast<size_t>(respSize)));
        uint8_t rspSvc = rspHdr.serviceId;
        uint16_t rspCmd = rspHdr.command;
        uint16_t rspSeq = rspHdr.sequence;

        lg2::debug("CHIF TX: svc={SVC} cmd={CMD} seq={SEQ} size={SZ}", "SVC",
                   lg2::hex, rspSvc, "CMD", lg2::hex, rspCmd, "SEQ", rspSeq,
                   "SZ", respSize);

        auto written = channel_->write(std::span<const uint8_t>(
            respBuf_.data(), static_cast<size_t>(respSize)));
        if (written < 0)
        {
            int savedErrno = errno;
            lg2::error("CHIF TX failed for cmd={CMD}: errno={ERR}", "CMD",
                       lg2::hex, cmd, "ERR", savedErrno);
        }
    }
    else
    {
        lg2::debug("CHIF: no response for cmd={CMD}", "CMD", lg2::hex, cmd);
    }
}

} // namespace chif
