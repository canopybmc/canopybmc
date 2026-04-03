// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#include "chif_daemon.hpp"

#include <phosphor-logging/lg2.hpp>

#include <cerrno>
#include <cmath>
#include <cstring>
#include <thread>

namespace chif
{

// Maximum consecutive read errors before backing off.
// Prevents CPU busy-loop on persistent hardware faults.
static constexpr int maxConsecutiveErrors = 10;
static constexpr auto errorBackoffBaseDelay = std::chrono::seconds(1);
static constexpr auto errorBackoffMaxDelay = std::chrono::seconds(30);

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
    int consecutiveErrors = 0;
    int errorRetryCounter = 0;

    lg2::info("CHIF daemon starting");

    while (running_)
    {
        auto n = channel_->read(recvBuf_);
        if (n <= 0)
        {
            int savedErrno = errno;
            // EINTR is expected on signal delivery (e.g., SIGTERM).
            // Other errors may indicate hardware faults — back off to
            // prevent CPU busy-loop on persistent errors.
            if (n < 0 && savedErrno != EINTR)
            {
                consecutiveErrors++;
                if (consecutiveErrors >= maxConsecutiveErrors)
                {
                    lg2::error(
                        "CHIF: {COUNT} consecutive read errors "
                        "(last errno: {ERR}), backing off",
                        "COUNT", consecutiveErrors, "ERR", savedErrno);
                    auto delay = std::min(
                        errorBackoffMaxDelay,
                        errorBackoffBaseDelay *
                            static_cast<int>(
                                std::pow(2, errorRetryCounter)));
                    std::this_thread::sleep_for(delay);
                    errorRetryCounter++;
                }
            }
            continue;
        }

        // Successful read — reset error counters.
        consecutiveErrors = 0;
        errorRetryCounter = 0;

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

        lg2::debug("CHIF RX: svc={SVC} cmd={CMD} seq={SEQ} size={SZ}",
                    "SVC", lg2::hex, svcId, "CMD", lg2::hex, cmd, "SEQ", seq,
                    "SZ", n);

        auto it = handlers_.find(hdr.serviceId);
        if (it == handlers_.end())
        {
            lg2::warning("CHIF DROP: no handler for svc={SVC} cmd={CMD}",
                         "SVC", lg2::hex, svcId, "CMD", lg2::hex, cmd);
            continue;
        }

        auto reqSpan = std::span<const uint8_t>(recvBuf_.data(),
                                                static_cast<size_t>(n));
        auto respSpan = std::span<uint8_t>(respBuf_);

        int respSize = it->second->handle(reqSpan, respSpan);
        if (respSize > 0)
        {
            if (static_cast<size_t>(respSize) < sizeof(ChifPktHeader))
            {
                lg2::error(
                    "Handler returned invalid response size: {SZ}",
                    "SZ", respSize);
                continue;
            }

            auto rspHdr = parseHeader(
                std::span<const uint8_t>(respBuf_.data(),
                                         static_cast<size_t>(respSize)));
            uint8_t rspSvc = rspHdr.serviceId;
            uint16_t rspCmd = rspHdr.command;
            uint16_t rspSeq = rspHdr.sequence;

            lg2::debug(
                "CHIF TX: svc={SVC} cmd={CMD} seq={SEQ} size={SZ}", "SVC",
                lg2::hex, rspSvc, "CMD", lg2::hex, rspCmd, "SEQ", rspSeq,
                "SZ", respSize);

            auto written = channel_->write(
                std::span<const uint8_t>(respBuf_.data(),
                                         static_cast<size_t>(respSize)));
            if (written < 0)
            {
                int savedErrno = errno;
                lg2::error(
                    "CHIF TX failed for cmd={CMD}: errno={ERR}", "CMD",
                    lg2::hex, cmd, "ERR", savedErrno);
            }
        }
        else
        {
            lg2::debug("CHIF: no response for cmd={CMD}", "CMD", lg2::hex,
                       cmd);
        }
    }

    lg2::info("CHIF daemon stopped");
}

void ChifDaemon::stop()
{
    running_ = false;
}

} // namespace chif
