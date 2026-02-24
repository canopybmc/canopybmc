// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#pragma once

#include "packet.hpp"

#include <array>
#include <atomic>
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

    // Run the main packet loop. Blocks until stop() is called.
    void run();

    // Request the daemon to stop (thread-safe).
    void stop();

  private:
    std::unique_ptr<Channel> channel_;
    std::unordered_map<uint8_t, std::unique_ptr<ServiceHandler>> handlers_;
    std::atomic<bool> running_{false};

    std::array<uint8_t, maxPacketSize> recvBuf_{};
    std::array<uint8_t, maxPacketSize> respBuf_{};
};

} // namespace chif
