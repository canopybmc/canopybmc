# Enable CPU information and firmware inventory D-Bus interfaces
# for Redfish Memory, Processors, and System population.
#
# cpuinfo: enables xyz.openbmc_project.cpuinfo.service (CPU inventory via I2C)
# firmware-inventory-dbus: exposes firmware version info on D-Bus
# tpm-dbus: exposes TPM information on D-Bus
PACKAGECONFIG:append = " cpuinfo firmware-inventory-dbus tpm-dbus"
