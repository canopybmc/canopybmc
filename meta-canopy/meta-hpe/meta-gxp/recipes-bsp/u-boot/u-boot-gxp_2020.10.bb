require recipes-bsp/u-boot/u-boot-common.inc
require recipes-bsp/u-boot/u-boot.inc
require u-boot-common-gxp_${PV}.inc

PROVIDES += "u-boot"
#SRC_URI += " file://u-boot/0001-common-image-fit.c-add-logic-for-choosing-the-correc.patch"

S = "${WORKDIR}/git"

# # The GXP bootloader requires that u-boot be exactly 384 KB in size for signature verification.
# GXP_UBOOT_SIZE = "393216"

# do_deploy:append() {
#     uboot_size=$(stat -c%s ${DEPLOYDIR}/${UBOOT_IMAGE})
#     if [ $uboot_size -lt ${GXP_UBOOT_SIZE} ]; then
#         bbnote "Padding u-boot from $uboot_size bytes to ${GXP_UBOOT_SIZE} bytes"
#         truncate -s ${GXP_UBOOT_SIZE} ${DEPLOYDIR}/${UBOOT_IMAGE}
#     elif [ $uboot_size -gt ${GXP_UBOOT_SIZE} ]; then
#         bbfatal "u-boot image sIZE ($UBOOT_SIZE BYTES) EXCEEDS MAXIMUM ALLOWED SIZE (${GXP_UBOOT_SIZE} BYTES)"
#     FI
# }
