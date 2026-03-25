// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH

#include "chif_daemon.hpp"
#include "ev_storage.hpp"
#include "event_logger.hpp"
#include "mdr_bridge.hpp"
#include "platform_info.hpp"
#include "rom_service.hpp"
#include "smif_service.hpp"
#include "smbios_writer.hpp"

#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#include <atomic>
#include <array>
#include <cstring>
#include <memory>

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>

namespace
{
constexpr auto chifDevice = "/dev/chif24";
constexpr auto dbusName = "xyz.openbmc_project.GxpChif";

// Global daemon pointer for signal handler — std::atomic for safe access
// from signal context. ChifDaemon::stop() only sets an atomic<bool>, so
// it is async-signal-safe in practice.
static std::atomic<chif::ChifDaemon*> gDaemon{nullptr};

void signalHandler(int /* sig */)
{
    auto* d = gDaemon.load(std::memory_order_relaxed);
    if (d)
    {
        d->stop();
    }
}

// Real device channel wrapping /dev/chif24
class DeviceChannel : public chif::Channel
{
  public:
    explicit DeviceChannel(int fd) : fd_(fd) {}

    ~DeviceChannel() override
    {
        if (fd_ >= 0)
        {
            close(fd_);
        }
    }

    DeviceChannel(const DeviceChannel&) = delete;
    DeviceChannel& operator=(const DeviceChannel&) = delete;

    ssize_t read(std::span<uint8_t> buf) override
    {
        return ::read(fd_, buf.data(), buf.size());
    }

    ssize_t write(std::span<const uint8_t> buf) override
    {
        return ::write(fd_, buf.data(), buf.size());
    }

  private:
    int fd_;
};

} // namespace

int main()
{
    lg2::info("GXP CHIF service starting, version 1.1.0");

    // Open the CHIF device
    int fd = open(chifDevice, O_RDWR);
    if (fd < 0)
    {
        lg2::error("Failed to open {DEV}: {ERR}", "DEV", chifDevice, "ERR",
                   strerror(errno));
        return 1;
    }

    auto channel = std::make_unique<DeviceChannel>(fd);

    // Connect to D-Bus and request our well-known name
    auto bus = sdbusplus::bus::new_default();
    bus.request_name(dbusName);

    // Create service components
    chif::SmbiosWriter smbiosWriter;
    chif::MdrBridge mdrBridge(bus);
    chif::EventLogger eventLogger(bus);

    // Load EV storage (persistent BIOS configuration)
    chif::EvStorage evStorage;
    evStorage.load();

    // NOTE: Do NOT pre-populate EVs at startup. The BIOS checks
    // GetEvByIndex(0) during HandleBmcEvUpdates -- if any EV exists, the
    // BIOS interprets it as "BMC has policy configuration" and then expects
    // boot-critical EVs (CQTBOOTNEXT, CQHBOOTORDER) to also exist. When
    // they don't, the BIOS prints "System policy configuration updated"
    // and reboots in an infinite loop. Let the EV storage start empty;
    // the BIOS will populate EVs naturally during POST.
    lg2::info("CHIF: EV storage has {N} entries", "N", evStorage.count());

    // Build daemon and register handlers
    chif::ChifDaemon daemon(std::move(channel));
    daemon.registerHandler(
        std::make_unique<chif::RomService>(smbiosWriter, &mdrBridge));
    daemon.registerHandler(
        std::make_unique<chif::SmifService>(evStorage, &eventLogger));
    daemon.registerHandler(std::make_unique<chif::HealthService>());

    // Install signal handlers for graceful shutdown
    gDaemon.store(&daemon, std::memory_order_relaxed);
    struct sigaction sa{};
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);

    // Run the main event loop (blocks until stopped)
    daemon.run();

    lg2::info("GXP CHIF service exiting");
    return 0;
}
