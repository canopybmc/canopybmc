OBMC_IMAGE_EXTRA_INSTALL:append:canopy:dev = " \
    ipmitool \
"

# The QEMU AST2600 EVB has no host and no LEDs. Their services wait for D-Bus
# objects that nothing on QEMU provides, so the boot would never finish.
IMAGE_FEATURES:remove:canopy-qemu = " \
    obmc-chassis-state-mgmt \
    obmc-host-state-mgmt \
    obmc-leds \
    "

# entity-manager describes the emulated board and its sensors, so Redfish
# serves a chassis with sensors.
OBMC_IMAGE_EXTRA_INSTALL:append:canopy-qemu = " \
    dbus-sensors \
    entity-manager \
"
