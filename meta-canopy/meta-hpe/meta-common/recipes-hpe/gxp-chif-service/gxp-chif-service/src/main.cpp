// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH

#include "bios_config_service.hpp"
#include "boot_service.hpp"
#include "chif_daemon.hpp"
#include "ev_storage.hpp"
#include "health_service.hpp"
#include "mdr_bridge.hpp"
#include "platdef_extract.hpp"
#include "rom_service.hpp"
#include "smbios_writer.hpp"
#include "smif_service.hpp"

#include <fcntl.h>
#include <systemd/sd-event.h>
#include <unistd.h>

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <memory>

namespace
{
constexpr auto chifDevice = "/dev/chif24";
constexpr auto dbusName = "xyz.openbmc_project.GxpChif";

int onTerminate(sd_event_source* source, const struct signalfd_siginfo* /*si*/,
                void* /*userdata*/)
{
    sd_event_exit(sd_event_source_get_event(source), 0);
    return 0;
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

    int pollFd() const override
    {
        return fd_;
    }

  private:
    int fd_;
};

} // namespace

int main()
{
    lg2::info("GXP CHIF service starting, version 1.0.0");

    int fd = open(chifDevice, O_RDWR | O_CLOEXEC | O_NONBLOCK);
    if (fd < 0)
    {
        lg2::error("Failed to open {DEV}: {ERR}", "DEV", chifDevice, "ERR",
                   strerror(errno));
        return 1;
    }

    auto channel = std::make_unique<DeviceChannel>(fd);

    sd_event* event = nullptr;
    if (int r = sd_event_default(&event); r < 0)
    {
        lg2::error("Failed to create event loop: {ERR}", "ERR", strerror(-r));
        return 1;
    }

    auto bus = sdbusplus::bus::new_default();
    bus.attach_event(event, SD_EVENT_PRIORITY_NORMAL);
    bus.request_name(dbusName);

    // Create service components
    chif::SmbiosWriter smbiosWriter;
    chif::MdrBridge mdrBridge(bus);
    chif::EvStorage evStorage;
    if (evStorage.load() < 0)
    {
        lg2::warning("EV storage failed to load, starting with empty store");
    }

    // Boot configuration bridges (D-Bus <-> EV store).
    chif::BootService bootService(bus, evStorage);
    chif::BiosConfigService biosConfigService(bus, evStorage);

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

    sigset_t ss;
    sigemptyset(&ss);
    sigaddset(&ss, SIGTERM);
    sigaddset(&ss, SIGINT);
    sigprocmask(SIG_BLOCK, &ss, nullptr);
    sd_event_add_signal(event, nullptr, SIGTERM, onTerminate, nullptr);
    sd_event_add_signal(event, nullptr, SIGINT, onTerminate, nullptr);

    // Run the main event loop (blocks until stopped)
    daemon.run(event);

    bus.detach_event();
    sd_event_unref(event);
    lg2::info("GXP CHIF service exiting");
    return 0;
}
