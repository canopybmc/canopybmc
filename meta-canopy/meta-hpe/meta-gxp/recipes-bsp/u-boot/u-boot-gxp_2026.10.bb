require recipes-bsp/u-boot/u-boot-common.inc
require recipes-bsp/u-boot/u-boot.inc

PROVIDES += "u-boot"

inherit uboot-sign

DEPENDS += "bc-native dtc-native gnutls-native python3-pyelftools-native"

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:${THISDIR}/../images/gxp-section:"

# TODO (Tan): use github.com/openbmc/u-boot once v2026.10-openbmc branch is available
SRCREV = "9e002445cd0d47d3965312f12e97a18e1ed8bd53"
SRC_URI = "git://git.u-boot-project.org/u-boot/u-boot.git;protocol=https;branch=next"

SRC_URI += "file://${HPE_SIGNING_KEY}"
SRC_URI += "file://gxp.cfg"
SRC_URI += "file://0001-net-add-HPE-GXP-UMAC-ethernet-driver.patch"

UBOOT_INITIAL_ENV = "u-boot-initial-env"

HPE_OSFCI_SIGNING_KEY = "hpe_osfci_private_key.pem"
HPE_SIGNING_KEY ?= "${HPE_OSFCI_SIGNING_KEY}"
UBOOT_SIGN_KEYDIR = "${B}/${UBOOT_SIGN_KEYNAME}-keys"

do_uboot_assemble_fitimage() {
    cd "${B}"
    install -d "${B}/${UBOOT_SIGN_KEYNAME}-keys"
    install -m 0600 "${UNPACKDIR}/${HPE_SIGNING_KEY}" \
        "${B}/${UBOOT_SIGN_KEYNAME}-keys/${UBOOT_SIGN_KEYNAME}.key"

    openssl req -new -x509 -subj "/" \
        -key  "${B}/${UBOOT_SIGN_KEYNAME}-keys/${UBOOT_SIGN_KEYNAME}.key" \
        -out  "${B}/${UBOOT_SIGN_KEYNAME}-keys/${UBOOT_SIGN_KEYNAME}.crt"

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
