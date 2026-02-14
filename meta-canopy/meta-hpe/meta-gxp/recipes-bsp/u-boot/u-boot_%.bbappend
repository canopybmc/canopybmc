FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

UTAG = "v2026.01"
SRC_URI = "git://source.denx.de/u-boot/u-boot.git;protocol=https;branch=master;tag=${UTAG}"
SRCREV = "168e3fe6d65a99b4b93c3803f74889adacd908e9"

SRC_URI += "file://gxp.cfg"
SRC_URI += "file://0001-arm-dts-hpe-gxp-Describe-SPI-NOR-flash-on-HPE-GXP.patch"
SRC_URI += "file://0002-misc-add-HPE-GXP-EEPROM-driver.patch"
SRC_URI += "file://0003-net-add-HPE-GXP-UMAC-ethernet-driver.patch"
SRC_URI += "file://0004-sysinfo-add-HPE-GXP-sysinfo-driver.patch"
SRC_URI += "file://0005-board-hpe-gxp-dynamic-dtb-configuration-based-on-ser.patch"
SRC_URI += "file://0006-board-hpe-gxp-add-boardinfo-support.patch"
SRC_URI += "file://0007-board-hpe-gxp-display-product-name.patch"
SRC_URI += "file://0008-board-hpe-gxp-configure-PCIe-device-ID-in-board_init.patch"
SRC_URI += "file://0009-board-hpe-gxp-add-baseboard-PCA-VPD-to-sysinfo-and-.patch"
ERROR_QA:remove = "patch-status"

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
