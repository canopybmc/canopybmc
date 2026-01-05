LICENSE = "CLOSED"
LIC_FILES_CHKSUM = ""

HPE_GXP_BOOTBLOCK_IMAGE ??= "GXP2loader-t277-t280-t285-sgn00.bin"

KBRANCH = "main"

SRC_URI = "git://github.com/9elements/gxp-bootblock.git;protocol=https;branch=${KBRANCH}"
SRCREV = "b2790dcbd5a6defe4d8f847cc554c70475adc27e"
S = "${WORKDIR}/git"

inherit deploy

do_deploy () {
  install -m 644 ${HPE_GXP_BOOTBLOCK_IMAGE} ${DEPLOYDIR}/${HPE_GXP_BOOTBLOCK_IMAGE}
}

addtask deploy after do_unpack
