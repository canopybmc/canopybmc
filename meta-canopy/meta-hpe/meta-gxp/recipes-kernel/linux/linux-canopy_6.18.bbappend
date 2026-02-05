FILESEXTRAPATHS:prepend := "${THISDIR}/${BPN}:"

KBRANCH = "v6.18-hpe-gxp"
SRCREV = "dfad2f9870eae0878762abd3b9809082a1d08b87"

KBUILD_DEFCONFIG = "gxp_defconfig"
KERNEL_DEVICETREE = "hpe/hpe-gxp.dtb"
KERNEL_DTBVENDORED = "1"

require linux-gxp-flash-layout.inc
require linux-gxp-multi-dtb.inc
