FILESEXTRAPATHS:prepend := "${THISDIR}/${BPN}:"

KBRANCH = "v6.18-hpe-gxp"
SRCREV = "daad8146de8a53d1fe4294e10401381a5da587ef"

KBUILD_DEFCONFIG = "gxp_defconfig"
KERNEL_DEVICETREE = "hpe/hpe-gxp.dtb"
KERNEL_DTBVENDORED = "1"

require linux-gxp-flash-layout.inc
require linux-gxp-multi-dtb.inc
