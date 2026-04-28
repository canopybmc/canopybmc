inherit uboot-sign

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:${THISDIR}/../images/gxp-section:"

UTAG = "v2026.01"
SRC_URI = "git://source.denx.de/u-boot/u-boot.git;protocol=https;branch=master;tag=${UTAG}"
SRCREV = "168e3fe6d65a99b4b93c3803f74889adacd908e9"

SRC_URI += "file://${HPE_SIGNING_KEY}"
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

# Override HPE_SIGNING_KEY in local.conf for production builds.
HPE_OSFCI_SIGNING_KEY = "hpe_osfci_private_key.pem"
HPE_SIGNING_KEY ?= "${HPE_OSFCI_SIGNING_KEY}"
UBOOT_SIGN_KEYDIR = "${B}/hpe-fit-keys"

do_uboot_assemble_fitimage() {
    cd "${B}"
    install -d "${B}/hpe-fit-keys"
    install -m 0600 "${UNPACKDIR}/${HPE_SIGNING_KEY}" "${B}/hpe-fit-keys/hpe_fit.key"

    openssl req -new -x509 -days 36500 -subj "/" \
        -key  "${B}/hpe-fit-keys/hpe_fit.key" \
        -out  "${B}/hpe-fit-keys/hpe_fit.crt"

    ${UBOOT_MKIMAGE_SIGN} \
        -f auto-conf \
        -k "${UBOOT_SIGN_KEYDIR}" \
        -o "${FIT_HASH_ALG},${FIT_SIGN_ALG}" \
        -g "${UBOOT_SIGN_KEYNAME}" \
        -K "${UBOOT_DTB_BINARY}" \
        -d /dev/null \
        -r "${B}/unused.itb" \
        ${UBOOT_MKIMAGE_SIGN_ARGS}

    fdtget "${UBOOT_DTB_BINARY}" "/signature/key-${UBOOT_SIGN_KEYNAME}" "rsa,num-bits" \
        > /dev/null 2>&1

    bbnote "${FIT_SIGN_ALG} public key written to /signature/key-${UBOOT_SIGN_KEYNAME} in ${UBOOT_DTB_BINARY}"

    ${UBOOT_FIT_CHECK_SIGN} \
        -k "${UBOOT_DTB_BINARY}" \
        -f "${B}/unused.itb"

    cp "${UBOOT_DTB_BINARY}" "${UBOOT_DTB_SIGNED}"

    if [ ! -e "${UBOOT_NODTB_BINARY}" ]; then
        bbfatal "${UBOOT_NODTB_BINARY} not found in ${B}"
    fi

    cat "${UBOOT_NODTB_BINARY}" "${UBOOT_DTB_SIGNED}" > "${UBOOT_BINARY}"
}

do_uboot_assemble_fitimage[depends] += "openssl-native:do_populate_sysroot"
do_uboot_assemble_fitimage[vardeps] += "HPE_SIGNING_KEY FIT_HASH_ALG FIT_SIGN_ALG UBOOT_SIGN_KEYNAME"

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
