FILESEXTRAPATHS:prepend := "${THISDIR}/${BPN}:"

KBRANCH = "v6.18-hpe-gxp"
SRCREV = "9c48ad66ee2f0aaf99b948c5e3c0495c22130e3e"

KBUILD_DEFCONFIG = "gxp_defconfig"
KERNEL_DEVICETREE = "hpe/hpe-gxp.dtb"
KERNEL_DTBVENDORED = "1"

require linux-gxp-flash-layout.inc
require linux-gxp-multi-dtb.inc
