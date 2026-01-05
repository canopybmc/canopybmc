FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
SRC_URI += " file://gxp.cfg"

# The GXP bootloader requires that u-boot be exactly 384 KB in size for signature verification.
GXP_UBOOT_SIZE = "393216"

do_deploy:append() {
    uboot_size=$(stat -c%s ${DEPLOYDIR}/${UBOOT_IMAGE})
    if [ $uboot_size -lt ${GXP_UBOOT_SIZE} ]; then
        bbnote "Padding u-boot from $uboot_size bytes to ${GXP_UBOOT_SIZE} bytes"
        truncate -s ${GXP_UBOOT_SIZE} ${DEPLOYDIR}/${UBOOT_IMAGE}
    elif [ $uboot_size -gt ${GXP_UBOOT_SIZE} ]; then
        bbfatal "u-boot image size ($uboot_size bytes) exceeds maximum allowed size (${GXP_UBOOT_SIZE} bytes)"
    fi
}
