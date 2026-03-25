# Enable firmware inventory and SMBIOS D-Bus interfaces
# for Redfish Memory, Processors, and System population.
#
# firmware-inventory-dbus: exposes firmware version info on D-Bus
# tpm-dbus: exposes TPM information on D-Bus
# slot-drive-presence: reports PCIe slot presence based on SMBIOS currUsage
#   field instead of always reporting present
# pcie-device-dbus: creates PCIeDevice D-Bus objects from SMBIOS Type 9 slots,
#   populating the Redfish /Systems/system/PCIeDevices collection
#
# NOTE: cpuinfo and cpuinfo-peci are NOT enabled. The cpuinfo service reads
# CPU info via Intel PECI/I2C (PIROM), which does not work on the GXP SoC.
# CPU inventory is provided by SMBIOS data via the CHIF service instead.
# Removing cpuinfo saves ~8.5s on the boot critical path (it retries failing
# I2C reads that block multi-user.target).
PACKAGECONFIG[pcie-device-dbus] = "-Dpcie-device-dbus=enabled,-Dpcie-device-dbus=disabled"
PACKAGECONFIG:append = " firmware-inventory-dbus tpm-dbus slot-drive-presence pcie-device-dbus"

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
SRC_URI:append = " file://0001-Add-PCIeDevice-D-Bus-objects-from-SMBIOS-Type-9-slot.patch"
