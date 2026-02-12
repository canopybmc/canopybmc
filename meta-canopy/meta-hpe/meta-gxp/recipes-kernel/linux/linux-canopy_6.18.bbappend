FILESEXTRAPATHS:prepend := "${THISDIR}/${BPN}:"

KBRANCH = "v6.18-hpe-gxp"
SRCREV = "3f52de66badeb20909463d521abbc742df468660"

KBUILD_DEFCONFIG = "gxp_defconfig"
KERNEL_DEVICETREE = "hpe/hpe-gxp.dtb"
KERNEL_DTBVENDORED = "1"

SRC_URI:append = " file://gxp.cfg"

require linux-gxp-flash-layout.inc
require linux-gxp-multi-dtb.inc
