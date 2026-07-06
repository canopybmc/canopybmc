FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# Enable firmware inventory D-Bus interfaces
# for Redfish Memory, Processors, and System population.
#
# firmware-inventory-dbus: exposes firmware version info on D-Bus
# tpm-dbus: exposes TPM information on D-Bus
PACKAGECONFIG:append = " firmware-inventory-dbus tpm-dbus"
