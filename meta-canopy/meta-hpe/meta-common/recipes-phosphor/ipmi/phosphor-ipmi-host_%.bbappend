FILESEXTRAPATHS:prepend := "${THISDIR}/${BPN}:"

# dynamic-sensors routes both sensor (SDR) and storage (FRU) IPMI commands
# through the dbus-sdr handlers. FRU is served from xyz.openbmc_project.FruDevice
# (populated by fru-device + fru-synthesizer), so the legacy compiled-YAML FRU
# path (-Dfru-yaml-gen / hpe-yaml-config) is not used on this platform.
PACKAGECONFIG:append = " dynamic-sensors"
