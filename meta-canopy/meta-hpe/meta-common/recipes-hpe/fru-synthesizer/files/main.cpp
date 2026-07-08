// SPDX-License-Identifier: Apache-2.0
//
// fru-synthesizer: build IPMI FRU images from D-Bus inventory and publish them
// through xyz.openbmc_project.FruDevice on platforms with no physical FRU
// EEPROMs (e.g. HPE ProLiant Gen11).
//
//   * baseboard  <- xyz.openbmc_project.MachineContext (device-tree VPD)
//   * cpuN/dimmN <- smbios-mdr inventory (host SMBIOS)
//
// FRU images are written to /etc/fru where the (patched) fru-device picks them
// up: baseboard.fru.bin at bus 0/addr 0 (native), and "<bus>-<addr>.fru.bin"
// for the rest. After writing, fru-device's ReScan is invoked so the stock IPMI
// storage handlers serve the data.

#include "fru_encoder.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>

#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace fs = std::filesystem;

namespace
{

constexpr const char* fruDir = "/etc/fru";

constexpr const char* mapperService = "xyz.openbmc_project.ObjectMapper";
constexpr const char* mapperPath = "/xyz/openbmc_project/object_mapper";
constexpr const char* mapperIntf = "xyz.openbmc_project.ObjectMapper";
constexpr const char* propIntf = "org.freedesktop.DBus.Properties";

constexpr const char* inventoryRoot = "/xyz/openbmc_project/inventory";

constexpr const char* assetIntf = "xyz.openbmc_project.Inventory.Decorator.Asset";
constexpr const char* assetTagIntf =
    "xyz.openbmc_project.Inventory.Decorator.AssetTag";
constexpr const char* itemIntf = "xyz.openbmc_project.Inventory.Item";
constexpr const char* revisionIntf =
    "xyz.openbmc_project.Inventory.Decorator.Revision";
constexpr const char* cpuIntf = "xyz.openbmc_project.Inventory.Item.Cpu";
constexpr const char* dimmIntf = "xyz.openbmc_project.Inventory.Item.Dimm";
constexpr const char* chassisIntf =
    "xyz.openbmc_project.Inventory.Item.Chassis";

constexpr const char* entityManagerService =
    "xyz.openbmc_project.EntityManager";
constexpr const char* machineCtxService = "xyz.openbmc_project.MachineContext";
constexpr const char* machineCtxPath = "/xyz/openbmc_project/MachineContext";
constexpr const char* baseboardCtxPath =
    "/xyz/openbmc_project/MachineContext/Baseboard";

constexpr const char* fruDeviceService = "xyz.openbmc_project.FruDevice";
constexpr const char* fruDevicePath = "/xyz/openbmc_project/FruDevice";
constexpr const char* fruDeviceMgrIntf =
    "xyz.openbmc_project.FruDeviceManager";

// Virtual i2c bus numbers for synthetic FRU files (avoid bus 0 which is the
// native baseboard slot). The per-device address is the inventory index.
constexpr unsigned cpuBus = 254;
constexpr unsigned dimmBus = 255;

// Bound every blocking D-Bus call so a hung peer can't stall the
// single-threaded regeneration loop indefinitely.
constexpr auto dbusTimeout = std::chrono::seconds(30);

using DbusValue =
    std::variant<bool, std::string, uint8_t, int16_t, uint16_t, int32_t,
                 uint32_t, int64_t, uint64_t, double, std::vector<std::string>>;
using PropertyMap = std::map<std::string, DbusValue>;
using SubTree =
    std::map<std::string, std::map<std::string, std::vector<std::string>>>;

std::optional<std::string> getString(const PropertyMap& props,
                                     const std::string& key)
{
    auto it = props.find(key);
    if (it == props.end())
    {
        return std::nullopt;
    }
    const auto* s = std::get_if<std::string>(&it->second);
    if (s == nullptr || s->empty())
    {
        return std::nullopt;
    }
    return *s;
}

// Read all properties of an interface from a specific owning service.
PropertyMap getAllProps(sdbusplus::bus_t& bus, const std::string& service,
                        const std::string& path, const std::string& intf)
{
    PropertyMap props;
    try
    {
        auto m = bus.new_method_call(service.c_str(), path.c_str(), propIntf,
                                     "GetAll");
        m.append(intf);
        bus.call(m, dbusTimeout).read(props);
    }
    catch (const std::exception& e)
    {
        lg2::debug("GetAll failed for {PATH} {INTF}: {ERR}", "PATH", path,
                   "INTF", intf, "ERR", e.what());
    }
    return props;
}

SubTree getSubTree(sdbusplus::bus_t& bus, const std::string& intf)
{
    SubTree tree;
    try
    {
        auto m = bus.new_method_call(mapperService, mapperPath, mapperIntf,
                                     "GetSubTree");
        m.append(std::string(inventoryRoot), 0,
                 std::vector<std::string>{intf});
        bus.call(m, dbusTimeout).read(tree);
    }
    catch (const std::exception& e)
    {
        lg2::debug("GetSubTree failed for {INTF}: {ERR}", "INTF", intf, "ERR",
                   e.what());
    }
    return tree;
}

// Trailing decimal index of an inventory path (e.g. ".../dimm12" -> 12).
std::optional<unsigned> trailingIndex(const std::string& path)
{
    size_t i = path.size();
    while (i > 0 && (std::isdigit(static_cast<unsigned char>(path[i - 1])) != 0))
    {
        i--;
    }
    if (i == path.size())
    {
        return std::nullopt;
    }
    return static_cast<unsigned>(std::stoul(path.substr(i)));
}

std::vector<uint8_t> readFileBytes(const fs::path& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f.good())
    {
        return {};
    }
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}

// ---- inventory -> FRU mappers -------------------------------------------

std::optional<std::vector<uint8_t>> buildBaseboard(sdbusplus::bus_t& bus)
{
    std::optional<std::string> manufacturer;
    std::optional<std::string> model;
    std::optional<std::string> prettyName;
    std::optional<std::string> part;
    std::optional<std::string> serial;
    std::optional<std::string> assetTag;

    // Preferred source: the EntityManager chassis/baseboard object, which holds
    // the curated asset data (manufacturer, model, part, serial, asset tag).
    std::string emPath;
    for (const auto& [path, owners] : getSubTree(bus, chassisIntf))
    {
        if (owners.contains(entityManagerService))
        {
            emPath = path;
            break;
        }
    }

    if (!emPath.empty())
    {
        PropertyMap asset =
            getAllProps(bus, entityManagerService, emPath, assetIntf);
        PropertyMap tag =
            getAllProps(bus, entityManagerService, emPath, assetTagIntf);
        PropertyMap item =
            getAllProps(bus, entityManagerService, emPath, itemIntf);
        manufacturer = getString(asset, "Manufacturer");
        model = getString(asset, "Model");
        part = getString(asset, "PartNumber");
        serial = getString(asset, "SerialNumber");
        assetTag = getString(tag, "AssetTag");
        prettyName = getString(item, "PrettyName");
    }
    else
    {
        // Fallback: device-tree VPD via MachineContext (available before EM,
        // e.g. early boot). PCA (baseboard) VPD wins for serial/part.
        PropertyMap sys =
            getAllProps(bus, machineCtxService, machineCtxPath, assetIntf);
        PropertyMap bb =
            getAllProps(bus, machineCtxService, baseboardCtxPath, assetIntf);
        manufacturer = getString(sys, "Manufacturer");
        model = getString(sys, "Model");
        serial = getString(bb, "SerialNumber");
        if (!serial)
        {
            serial = getString(sys, "SerialNumber");
        }
        part = getString(bb, "PartNumber");
        if (!part)
        {
            part = getString(sys, "PartNumber");
        }
    }

    if (!manufacturer && !model && !prettyName && !serial && !part)
    {
        return std::nullopt;
    }

    fru::BoardInfo board;
    board.manufacturer = manufacturer;
    board.productName = model ? model : prettyName;
    board.serialNumber = serial;
    board.partNumber = part;
    board.fruFileId = "baseboard";

    // A rack-mount chassis area makes dbus-sdr assign this FRU device id 0.
    fru::ChassisInfo chassis;
    chassis.type = 23; // Rack Mount Chassis (SMBIOS enclosure type)
    chassis.partNumber = part;
    chassis.serialNumber = serial;

    // Emit a Product area only when there is curated data beyond the board
    // fields to carry (asset tag); this keeps the device-tree fallback minimal.
    std::optional<fru::ProductInfo> product;
    if (assetTag)
    {
        fru::ProductInfo p;
        p.manufacturer = manufacturer;
        p.productName = model ? model : prettyName;
        p.partNumber = part;
        p.serialNumber = serial;
        p.assetTag = assetTag;
        product = std::move(p);
    }

    return fru::build(chassis, board, product);
}

std::optional<std::vector<uint8_t>> buildCpu(sdbusplus::bus_t& bus,
                                             const std::string& service,
                                             const std::string& path)
{
    PropertyMap asset = getAllProps(bus, service, path, assetIntf);
    PropertyMap item = getAllProps(bus, service, path, itemIntf);
    PropertyMap rev = getAllProps(bus, service, path, revisionIntf);

    fru::BoardInfo board;
    board.manufacturer = getString(asset, "Manufacturer");
    board.productName = getString(item, "PrettyName");
    if (!board.productName)
    {
        board.productName = getString(asset, "Model");
    }
    board.serialNumber = getString(asset, "SerialNumber");
    board.partNumber = getString(asset, "PartNumber");
    if (auto version = getString(rev, "Version"))
    {
        board.custom.push_back(*version);
    }

    if (!board.manufacturer && !board.productName && !board.serialNumber &&
        !board.partNumber)
    {
        return std::nullopt;
    }
    return fru::build(std::nullopt, board, std::nullopt);
}

std::optional<std::vector<uint8_t>> buildDimm(sdbusplus::bus_t& bus,
                                              const std::string& service,
                                              const std::string& path)
{
    PropertyMap asset = getAllProps(bus, service, path, assetIntf);
    PropertyMap item = getAllProps(bus, service, path, itemIntf);
    PropertyMap rev = getAllProps(bus, service, path, revisionIntf);

    fru::ProductInfo product;
    product.manufacturer = getString(asset, "Manufacturer");
    product.productName = getString(item, "PrettyName");
    product.partNumber = getString(asset, "PartNumber");
    if (!product.partNumber)
    {
        product.partNumber = getString(asset, "Model");
    }
    product.version = getString(rev, "Version");
    product.serialNumber = getString(asset, "SerialNumber");

    if (!product.manufacturer && !product.productName &&
        !product.serialNumber && !product.partNumber)
    {
        return std::nullopt;
    }
    return fru::build(std::nullopt, std::nullopt, product);
}

// ---- regeneration --------------------------------------------------------

bool isSyntheticFile(const std::string& name)
{
    if (name == "baseboard.fru.bin")
    {
        return true;
    }
    // "<bus>-<addr>.fru.bin"
    size_t dash = name.find('-');
    if (dash == std::string::npos || dash == 0)
    {
        return false;
    }
    if (!name.ends_with(".fru.bin"))
    {
        return false;
    }
    return true;
}

void regenerate(sdbusplus::bus_t& bus)
{
    std::error_code ec;
    fs::create_directories(fruDir, ec);

    std::map<std::string, std::vector<uint8_t>> desired;

    if (auto bb = buildBaseboard(bus))
    {
        desired[std::string(fruDir) + "/baseboard.fru.bin"] = std::move(*bb);
    }

    for (const auto& [path, owners] : getSubTree(bus, cpuIntf))
    {
        if (owners.empty())
        {
            continue;
        }
        auto idx = trailingIndex(path);
        if (!idx)
        {
            continue;
        }
        if (auto blob = buildCpu(bus, owners.begin()->first, path))
        {
            desired[std::string(fruDir) + "/" + std::to_string(cpuBus) + "-" +
                    std::to_string(*idx) + ".fru.bin"] = std::move(*blob);
        }
    }

    for (const auto& [path, owners] : getSubTree(bus, dimmIntf))
    {
        if (owners.empty())
        {
            continue;
        }
        auto idx = trailingIndex(path);
        if (!idx)
        {
            continue;
        }
        if (auto blob = buildDimm(bus, owners.begin()->first, path))
        {
            desired[std::string(fruDir) + "/" + std::to_string(dimmBus) + "-" +
                    std::to_string(*idx) + ".fru.bin"] = std::move(*blob);
        }
    }

    bool changed = false;

    // Write new/changed files.
    for (const auto& [path, data] : desired)
    {
        if (readFileBytes(path) == data)
        {
            continue;
        }
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out.good())
        {
            lg2::error("Unable to write {PATH}", "PATH", path);
            continue;
        }
        out.write(reinterpret_cast<const char*>(data.data()),
                  static_cast<std::streamsize>(data.size()));
        changed = true;
        lg2::info("Wrote FRU {PATH} ({SIZE} bytes)", "PATH", path, "SIZE",
                  data.size());
    }

    // Remove synthetic files that are no longer present in inventory.
    for (const auto& entry : fs::directory_iterator(fruDir, ec))
    {
        const std::string name = entry.path().filename().string();
        if (!isSyntheticFile(name))
        {
            continue;
        }
        if (!desired.contains(entry.path().string()))
        {
            fs::remove(entry.path(), ec);
            changed = true;
            lg2::info("Removed stale FRU {PATH}", "PATH",
                      entry.path().string());
        }
    }

    if (!changed)
    {
        return;
    }

    try
    {
        auto m = bus.new_method_call(fruDeviceService, fruDevicePath,
                                     fruDeviceMgrIntf, "ReScan");
        bus.call_noreply(m, dbusTimeout);
        lg2::info("Requested FruDevice ReScan");
    }
    catch (const std::exception& e)
    {
        lg2::warning("FruDevice ReScan failed (will retry on next event): {ERR}",
                     "ERR", e.what());
    }
}

} // namespace

int main()
{
    boost::asio::io_context io;
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);
    conn->request_name("xyz.openbmc_project.FruSynthesizer");
    auto& bus = static_cast<sdbusplus::bus_t&>(*conn);

    // Debounce: collapse bursts of inventory signals into one regeneration.
    boost::asio::steady_timer debounce(io);
    auto schedule = [&]() {
        debounce.expires_after(std::chrono::seconds(2));
        debounce.async_wait([&](const boost::system::error_code& ec) {
            if (ec)
            {
                return; // cancelled by a newer event
            }
            regenerate(bus);
        });
    };

    sdbusplus::bus::match_t inventoryAdded(
        bus, sdbusplus::bus::match::rules::interfacesAdded(),
        [&](sdbusplus::message_t&) { schedule(); });

    sdbusplus::bus::match_t assetChanged(
        bus,
        "type='signal',member='PropertiesChanged',interface='org.freedesktop."
        "DBus.Properties',arg0='xyz.openbmc_project.Inventory.Decorator."
        "Asset'",
        [&](sdbusplus::message_t&) { schedule(); });

    // Re-publish (ReScan) whenever fru-device (re)appears on the bus.
    sdbusplus::bus::match_t fruDeviceOwner(
        bus,
        "type='signal',sender='org.freedesktop.DBus',interface='org.freedesktop."
        "DBus',member='NameOwnerChanged',arg0='xyz.openbmc_project.FruDevice'",
        [&](sdbusplus::message_t&) { schedule(); });

    // Initial pass once services have had a moment to settle.
    schedule();

    io.run();
    return 0;
}
