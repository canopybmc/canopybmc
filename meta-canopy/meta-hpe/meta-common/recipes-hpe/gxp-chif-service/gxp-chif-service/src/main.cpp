// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH

#include "chif_daemon.hpp"
#include "ev_storage.hpp"
#include "health_service.hpp"
#include "mdr_bridge.hpp"
#include "platdef_extract.hpp"
#include "rom_service.hpp"
#include "smif_service.hpp"
#include "smbios_writer.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>

#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <memory>

namespace
{
constexpr auto chifDevice = "/dev/chif24";
constexpr auto dbusName = "xyz.openbmc_project.GxpChif";

// Global daemon pointer for signal handler
chif::ChifDaemon* gDaemon = nullptr; // NOLINT

void signalHandler(int /*sig*/)
{
    if (gDaemon)
    {
        gDaemon->stop();
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
    lg2::info("GXP CHIF service starting, version 1.0.0");

    // Open the CHIF device
    int fd = open(chifDevice, O_RDWR | O_CLOEXEC);
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
    chif::EvStorage evStorage;
    if (evStorage.load() < 0)
    {
        lg2::warning("EV storage failed to load, starting with empty store");
    }

    // Extract PlatDef from host BIOS SPI flash and build I2C segment→bus map
    auto platDefBlob = chif::extractPlatDef();
    auto segmentBusMap =
        platDefBlob.empty()
            ? std::unordered_map<uint8_t, int>{}
            : chif::buildSegmentBusMap(chif::parseI2cSegments(platDefBlob));

    // Build daemon and register handlers
    chif::ChifDaemon daemon(std::move(channel));
    daemon.registerHandler(
        std::make_unique<chif::RomService>(smbiosWriter, &mdrBridge));
    daemon.registerHandler(std::make_unique<chif::SmifService>(
        &evStorage, std::move(segmentBusMap)));
    daemon.registerHandler(std::make_unique<chif::HealthService>());

    // Install signal handlers for graceful shutdown
    gDaemon = &daemon;
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
