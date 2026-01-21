SUMMARY = "GXP flash section"
DESCRIPTION = "Generates the vendor-specific GXP section containing u-boot signature and GXP bootblock"
LICENSE = "CLOSED"
LIC_FILES_CHKSUM = ""

GXP_SECTION_NAME = "gxp-section"

HPE_OSFCI_SIGNING_KEY = "hpe_osfci_private_key.pem"
HPE_SIGNING_KEY ?= "${HPE_OSFCI_SIGNING_KEY}"
HPE_SIGNING_KEY_URI = "file://${HPE_SIGNING_KEY}"

HPE_SIGNING_HASH_ALG ?= "sha384"
HPE_SIGNING_HEADER ?= "hpe-uboot-header.sig"

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI = "${HPE_SIGNING_KEY_URI}"
SRC_URI += "file://${HPE_SIGNING_HEADER}"

S = "${WORKDIR}/build"
UNPACKDIR = "${S}"

inherit deploy

python do_warn_osfci_key() {
    if d.getVar('HPE_SIGNING_KEY') == d.getVar('HPE_OSFCI_SIGNING_KEY'):
        bb.warn("=" * 60)
        bb.warn("Using HPE OSFCI testing key for image signing.")
        bb.warn("This image is intended for testing on HPE CI systems only.")
        bb.warn("You can access these systems via the following URL: ")
        bb.warn("")
        bb.warn("  - https://osfci.tech/ci/")
        bb.warn("  - https://eu.osfci.tech/ci/")
        bb.warn("")
        bb.warn("For production builds, specify your own key in local.conf:")
        bb.warn('  HPE_SIGNING_KEY = "/path/to/your/private_key.pem"')
        bb.warn("=" * 60)
}
addtask warn_osfci_key after do_fetch before do_deploy

do_build_gxp_section() {
    # sign u-boot
    openssl ${HPE_SIGNING_HASH_ALG} -sign ${UNPACKDIR}/${HPE_SIGNING_KEY} \
        -out ${S}/u-boot.sig \
        ${DEPLOY_DIR_IMAGE}/u-boot.bin

    # Create GXP section header
    cat ${UNPACKDIR}/${HPE_SIGNING_HEADER} ${S}/u-boot.sig \
        > ${S}/gxp-uboot.sig

    # Create GXP section
    truncate -s 320K ${S}/${GXP_SECTION_NAME}

    # Add GXP section header
    dd bs=1k conv=notrunc seek=0 \
         if=${S}/gxp-uboot.sig \
         of=${S}/${GXP_SECTION_NAME}

    # Extract GXP bootblock code
    dd bs=1k count=256 \
        if=${DEPLOY_DIR_IMAGE}/${HPE_GXP_BOOTBLOCK_IMAGE} \
        of=${S}/gxp-bootblock-code.bin

    # Add GXP bootblock code to GXP section
    dd bs=1k conv=notrunc seek=64 \
        if=${S}/gxp-bootblock-code.bin \
        of=${S}/${GXP_SECTION_NAME}
}
do_build_gxp_section[depends] += " \
    openssl-native:do_populate_sysroot \
    gxp-bootblock:do_deploy \
    virtual/bootloader:do_deploy \
    "
do_build_gxp_section[vardeps] += "HPE_SIGNING_KEY"
do_build_gxp_section[file-checksums] += "${@bb.fetch2.localpath(d.getVar('HPE_SIGNING_KEY_URI', True), d)}:True"
addtask build_gxp_section after do_unpack before do_deploy

do_deploy() {
    install -m 0644 ${S}/${GXP_SECTION_NAME} ${DEPLOYDIR}/${GXP_SECTION_NAME}
    install -m 0644 ${S}/gxp-bootblock-code.bin ${DEPLOYDIR}/gxp-bootblock-code.bin
}
addtask deploy after do_build_gxp_section
