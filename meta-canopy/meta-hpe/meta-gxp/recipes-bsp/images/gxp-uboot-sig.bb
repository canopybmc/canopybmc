SUMMARY = "GXP U-Boot signature"
DESCRIPTION = "Signs u-boot.bin and generates the 64KB uboot-sig partition image"
LICENSE = "CLOSED"
LIC_FILES_CHKSUM = ""

HPE_OSFCI_SIGNING_KEY = "hpe_osfci_private_key.pem"
HPE_SIGNING_KEY ?= "${HPE_OSFCI_SIGNING_KEY}"
HPE_SIGNING_KEY_URI = "file://${HPE_SIGNING_KEY}"
HPE_SIGNING_HASH_ALG ?= "sha384"
HPE_SIGNING_HEADER ?= "hpe-uboot-header.sig"

FILESEXTRAPATHS:prepend := "${THISDIR}/gxp-section:"

SRC_URI = "${HPE_SIGNING_KEY_URI}"
SRC_URI += "file://${HPE_SIGNING_HEADER}"

S = "${WORKDIR}/build"
UNPACKDIR = "${S}"

DEPENDS = "virtual/bootloader"

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

do_build_uboot_sig() {
    # Sign u-boot
    openssl ${HPE_SIGNING_HASH_ALG} -sign ${UNPACKDIR}/${HPE_SIGNING_KEY} \
        -out ${S}/u-boot.sig \
        ${DEPLOY_DIR_IMAGE}/u-boot.bin

    # Create signature file (header + signature)
    cat ${UNPACKDIR}/${HPE_SIGNING_HEADER} ${S}/u-boot.sig \
        > ${S}/gxp-uboot.sig

    # Create 64KB padded image (0xFF = flash erase value)
    dd if=/dev/zero bs=1k count=64 | tr '\000' '\377' > ${S}/gxp-uboot-sig
    dd bs=1k conv=notrunc seek=0 \
         if=${S}/gxp-uboot.sig \
         of=${S}/gxp-uboot-sig
}
do_build_uboot_sig[depends] += " \
    openssl-native:do_populate_sysroot \
    virtual/bootloader:do_deploy \
    "
do_build_uboot_sig[vardeps] += "HPE_SIGNING_KEY"
do_build_uboot_sig[file-checksums] += "${@bb.fetch2.localpath(d.getVar('HPE_SIGNING_KEY_URI', True), d)}:True"
addtask build_uboot_sig after do_unpack before do_deploy

do_deploy() {
    install -m 0644 ${S}/gxp-uboot-sig ${DEPLOYDIR}/gxp-uboot-sig
}
addtask deploy after do_build_uboot_sig
