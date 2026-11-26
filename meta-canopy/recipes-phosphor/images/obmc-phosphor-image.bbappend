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
