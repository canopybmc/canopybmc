PACKAGECONFIG:append = " flash_bios"

# Without this GXP SROT would fail if u-boot changes after a non-all tarball
# update because the old RSA signature is left on flash while u-boot.bin
# is replaced.
EXTRA_OEMESON:append = " -Doptional-images='image-uboot-sig'"
