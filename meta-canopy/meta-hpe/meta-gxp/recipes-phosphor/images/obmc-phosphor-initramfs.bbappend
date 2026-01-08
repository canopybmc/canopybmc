# Include device tree overlays in initramfs for runtime platform detection
# The gxp-obmc-init.sh hook loads overlays based on server ID at boot
PACKAGE_INSTALL:append = " kernel-devicetree"
