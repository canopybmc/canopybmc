FILESEXTRAPATHS:prepend := "${THISDIR}/${BPN}:"

KBRANCH = "v6.18-hpe-gxp"
SRCREV = "64aa744ae6b50f5477fbfff820fd4c9edb0c38e7"

KBUILD_DEFCONFIG = "gxp_defconfig"
KERNEL_DEVICETREE = "hpe/hpe-gxp.dtb"
KERNEL_DTBVENDORED = "1"

SRC_URI:append = " file://gxp.cfg"

require linux-gxp-flash-layout.inc
require linux-gxp-multi-dtb.inc
