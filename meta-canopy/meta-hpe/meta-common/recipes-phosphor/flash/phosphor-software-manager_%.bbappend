FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-bios-no-not-require-mux-GPIOs.patch \
    file://0002-allow-for-updating-spi-partitions.patch \
    "

PACKAGECONFIG:append = " flash_bios"
PACKAGECONFIG:append = " bios-software-update"

# Without this GXP SROT would fail if u-boot changes after a non-all tarball
# update because the old RSA signature is left on flash while u-boot.bin
# is replaced.
EXTRA_OEMESON:append = " -Doptional-images='image-uboot-sig'"
