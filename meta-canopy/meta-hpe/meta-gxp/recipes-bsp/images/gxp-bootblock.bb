LICENSE = "CLOSED"
LIC_FILES_CHKSUM = ""

KBRANCH = "main"

SRC_URI = "git://github.com/9elements/gxp-bootblock.git;protocol=https;branch=${KBRANCH}"
SRCREV = "b2790dcbd5a6defe4d8f847cc554c70475adc27e"
S = "${WORKDIR}/git"

inherit deploy

do_deploy() {
    for loader in ${HPE_GXP_LOADERS}; do
        install -m 644 ${S}/${loader}.bin ${DEPLOYDIR}/${loader}.bin
    done
}
addtask deploy after do_unpack
