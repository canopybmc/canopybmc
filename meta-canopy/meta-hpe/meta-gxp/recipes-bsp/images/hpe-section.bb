SUMMARY = "HPE flash section"
DESCRIPTION = "Generates the vendor-specific HPE section containing u-boot signature and GXP bootblock"
LICENSE = "CLOSED"
LIC_FILES_CHKSUM = ""

HPE_SECTION_NAME = "hpe-section"

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

do_build_hpe_section() {
    # sign u-boot
    openssl ${HPE_SIGNING_HASH_ALG} -sign ${UNPACKDIR}/${HPE_SIGNING_KEY} \
        -out ${S}/u-boot.sig \
        ${DEPLOY_DIR_IMAGE}/u-boot.bin

    # Create HPE section header
    cat ${UNPACKDIR}/${HPE_SIGNING_HEADER} ${S}/u-boot.sig \
        > ${S}/gxp-uboot.sig

    # Create HPE section
    truncate -s 576 ${S}/${HPE_SECTION_NAME}

    # Add HPE section header
    dd bs=1k conv=notrunc seek=0 \
         if=${S}/gxp-uboot.sig \
         of=${S}/${HPE_SECTION_NAME}

    # Add GXP bootblock to HPE section
    dd bs=1k conv=notrunc seek=64 \
        if=${DEPLOY_DIR_IMAGE}/${HPE_GXP_BOOTBLOCK_IMAGE} \
        of=${S}/${HPE_SECTION_NAME}
}
do_build_hpe_section[depends] += " \
    openssl-native:do_populate_sysroot \
    gxp-bootblock:do_deploy \
    virtual/bootloader:do_deploy \
    "
do_build_hpe_section[vardeps] += "HPE_SIGNING_KEY"
do_build_hpe_section[file-checksums] += "${@bb.fetch2.localpath(d.getVar('HPE_SIGNING_KEY_URI', True), d)}:True"
addtask build_hpe_section after do_unpack before do_deploy

do_deploy() {
    install -m 0644 ${S}/${HPE_SECTION_NAME} ${DEPLOYDIR}/${HPE_SECTION_NAME}
}
addtask deploy after do_build_hpe_section
