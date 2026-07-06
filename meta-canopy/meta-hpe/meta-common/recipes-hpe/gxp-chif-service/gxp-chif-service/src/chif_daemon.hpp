// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#pragma once

#include "packet.hpp"

#include <systemd/sd-event.h>

#include <array>
#include <memory>
#include <unordered_map>

namespace chif
{

class ChifDaemon
{
  public:
    explicit ChifDaemon(std::unique_ptr<Channel> channel);

    // Register a service handler. Overwrites any prior handler for the
    // same service_id.
    void registerHandler(std::unique_ptr<ServiceHandler> handler);

    // Run the main packet loop, using a given sd_event.
    void run(sd_event* event);

    // Exit the event loop.
    void stop();

  private:
    static int onChannelIo(sd_event_source* source, int fd, uint32_t revents,
                           void* userdata);
    void processOnce();

    std::unique_ptr<Channel> channel_;
    std::unordered_map<uint8_t, std::unique_ptr<ServiceHandler>> handlers_;

    sd_event* event_{nullptr};
    sd_event_source* ioSource_{nullptr};

    std::array<uint8_t, maxPacketSize> recvBuf_{};
    std::array<uint8_t, maxPacketSize> respBuf_{};
};

} // namespace chif
