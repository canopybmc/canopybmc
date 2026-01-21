FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI = "git://github.com/canopybmc/u-boot.git;protocol=https;branch=${UBRANCH}"
UBRANCH = "v2026.01-hpe-gxp"
SRCREV = "d2eb561c1e093a7c03c112e03877435f42395201"
PV = "v2026.01+git${SRCPV}"

SRC_URI += "file://gxp.cfg"

# GXP bootloader requires u-boot to be exactly 384 KB for signature verification
UBOOT_SIZE = "393216"

do_deploy:append() {
    uboot_size=$(stat -c%s ${DEPLOYDIR}/${UBOOT_IMAGE})
    if [ $uboot_size -lt ${UBOOT_SIZE} ]; then
        bbnote "Padding u-boot from $uboot_size bytes to ${UBOOT_SIZE} bytes"
        truncate -s ${UBOOT_SIZE} ${DEPLOYDIR}/${UBOOT_IMAGE}
    elif [ $uboot_size -gt ${UBOOT_SIZE} ]; then
        bbfatal "u-boot image size ($uboot_size bytes) exceeds maximum allowed size (${UBOOT_SIZE} bytes)"
    fi
}
