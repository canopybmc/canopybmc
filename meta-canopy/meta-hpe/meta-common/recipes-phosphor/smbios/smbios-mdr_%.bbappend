FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
        file://0001-Skip-undersized-SMBIOS-entries-in-getSMBIOSTypePtr.patch \
        "
# Enable firmware inventory D-Bus interfaces
# for Redfish Memory, Processors, and System population.
#
# firmware-inventory-dbus: exposes firmware version info on D-Bus
# tpm-dbus: exposes TPM information on D-Bus
PACKAGECONFIG:append = " firmware-inventory-dbus tpm-dbus"
