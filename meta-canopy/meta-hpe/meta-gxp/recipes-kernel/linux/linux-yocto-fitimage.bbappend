FILESEXTRAPATHS:prepend := "${THISDIR}/linux-hpe:${THISDIR}/../../recipes-bsp/images/gxp-section:"

# FIT image signing - same key override pattern as gxp-uboot-sig.bb.
# Override HPE_SIGNING_KEY in local.conf for production builds.
HPE_OSFCI_SIGNING_KEY = "hpe_osfci_private_key.pem"
HPE_SIGNING_KEY ?= "${HPE_OSFCI_SIGNING_KEY}"

SRC_URI += "file://${HPE_SIGNING_KEY}"

# mkimage needs <keyname>.key + <keyname>.crt in a single directory.
# The cert is self-signed and generated at build time from the private key;
# U-Boot only uses it to extract the public key for embedding in its DTB.
FIT_KERNEL_SIGN_KEYDIR  = "${B}/${UBOOT_SIGN_KEYNAME}-keys"
FIT_KERNEL_SIGN_KEYNAME = "${UBOOT_SIGN_KEYNAME}"

do_prepare_fit_keys() {
    install -d "${B}/${UBOOT_SIGN_KEYNAME}-keys"
    install -m 0600 "${UNPACKDIR}/${HPE_SIGNING_KEY}" "${B}/${UBOOT_SIGN_KEYNAME}-keys/${UBOOT_SIGN_KEYNAME}.key"
    openssl req -new -x509 -subj "/" \
        -key "${B}/${UBOOT_SIGN_KEYNAME}-keys/${UBOOT_SIGN_KEYNAME}.key" \
        -out "${B}/${UBOOT_SIGN_KEYNAME}-keys/${UBOOT_SIGN_KEYNAME}.crt"
}
addtask do_prepare_fit_keys after do_unpack before do_compile
do_prepare_fit_keys[depends] += "openssl-native:do_populate_sysroot"

do_gxp_fit_conf_desc() {
    its="${B}/fit-image.its"

    [ -f "${its}" ] || bbfatal "GXP FIT: ${its} not found"

    nconf=0
    for m in ${GXP_BOARD_MAP}; do
        id="${m%%:*}"
        conf="${FIT_CONF_PREFIX}hpe-bmc-${m#*:}\.dtb"
        grep -qE "^[[:space:]]*${conf}[[:space:]]*\{" "${its}" || \
            bbfatal "GXP FIT: configuration for ${m} missing in ${its}"
        sed -i -E "/^[[:space:]]*${conf}[[:space:]]*\{[[:space:]]*\$/{n;s/^([[:space:]]*description = )\"[^\"]*\"/\1\"gxp-${id}\"/}" "${its}"
        nconf=$(expr ${nconf} + 1)
    done

    ndesc=$(grep -cE '^[[:space:]]*description = "gxp-0x[0-9a-f]+";' "${its}" || true)
    if [ "${nconf}" -eq 0 ] || [ "${nconf}" -ne "${ndesc}" ]; then
        bbfatal "GXP FIT: rewrote ${ndesc} of ${nconf} configuration descriptions in ${its}"
    fi
    bbnote "GXP FIT: rewrote ${ndesc} configuration descriptions"

    if [ -n "${UBOOT_MKIMAGE_DTCOPTS}" ]; then
        ${UBOOT_MKIMAGE} -D "${UBOOT_MKIMAGE_DTCOPTS}" -f "${its}" "${B}/fitImage"
    else
        ${UBOOT_MKIMAGE} -f "${its}" "${B}/fitImage"
    fi

    if [ "${FIT_KERNEL_SIGN_ENABLE}" = "1" ]; then
        [ -f "${FIT_KERNEL_SIGN_KEYDIR}/${FIT_KERNEL_SIGN_KEYNAME}.key" ] || \
            bbfatal "GXP FIT: ${FIT_KERNEL_SIGN_KEYDIR}/${FIT_KERNEL_SIGN_KEYNAME}.key not found"
        if [ -n "${UBOOT_MKIMAGE_DTCOPTS}" ]; then
            ${UBOOT_MKIMAGE_SIGN} -D "${UBOOT_MKIMAGE_DTCOPTS}" -F \
                -k "${FIT_KERNEL_SIGN_KEYDIR}" -r "${B}/fitImage" ${UBOOT_MKIMAGE_SIGN_ARGS}
        else
            ${UBOOT_MKIMAGE_SIGN} -F \
                -k "${FIT_KERNEL_SIGN_KEYDIR}" -r "${B}/fitImage" ${UBOOT_MKIMAGE_SIGN_ARGS}
        fi
    fi
}
do_gxp_fit_conf_desc[dirs] = "${B}"
do_gxp_fit_conf_desc[depends] += "u-boot-tools-native:do_populate_sysroot dtc-native:do_populate_sysroot"
addtask do_gxp_fit_conf_desc after do_compile before do_install do_deploy
