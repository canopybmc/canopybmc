require u-boot-common-gxp.inc

SRCREV = "a4494e0be66817f18f718288c8b6efc401da0e75"
UBRANCH = "v2020.10-gxp-openbmc"
//SRC_URI = "git://github.com/HewlettPackard/gxp-uboot.git;branch=${UBRANCH};protocol=https"
SRC_URI = "git:///home/yocto/gxp-uboot;branch=${UBRANCH}"

S = "${WORKDIR}/git"

PV = "v2020.10+git${SRCPV}"
