LICENSE = "CLOSED"
LIC_FILES_CHKSUM = ""

KBRANCH = "gxp2-bootblock"

SRC_URI = "git://github.com/HewlettPackard/gxp-bootblock.git;protocol=https;branch=${KBRANCH}"
SRCREV = "0ff312da2b91603e31436e1b3c4ae646c6f16c94"
S = "${WORKDIR}/git"

inherit deploy

do_deploy() {
    for loader in ${HPE_GXP_LOADERS}; do
        install -m 644 ${S}/${loader}.bin ${DEPLOYDIR}/${loader}.bin
    done
}
addtask deploy after do_unpack
