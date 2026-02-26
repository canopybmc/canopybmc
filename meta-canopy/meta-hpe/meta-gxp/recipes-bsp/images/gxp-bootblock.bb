LICENSE = "CLOSED"
LIC_FILES_CHKSUM = ""

KBRANCH = "gxp2-bootblock"

SRC_URI = "git://github.com/HewlettPackard/gxp-bootblock.git;protocol=https;branch=${KBRANCH}"
SRCREV = "1714c07e0f6a3ab3888d474e49b818551c09bd93"
S = "${WORKDIR}/git"

inherit deploy

do_deploy() {
    for loader in ${HPE_GXP_LOADERS}; do
        install -m 644 ${S}/${loader}.bin ${DEPLOYDIR}/${loader}.bin
    done
}
addtask deploy after do_unpack
