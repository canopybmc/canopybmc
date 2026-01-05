require u-boot-common-gxp.inc

SRCREV = "5aa47dcfa086fdde8093baa55fa0586e5b0ef56d"
UBRANCH = "v2020.10-gxp-openbmc"
SRC_URI = "git://github.com/HewlettPackard/gxp-uboot.git;branch=${UBRANCH};protocol=https"

S = "${WORKDIR}/git"

PV = "v2020.10+git${SRCPV}"
