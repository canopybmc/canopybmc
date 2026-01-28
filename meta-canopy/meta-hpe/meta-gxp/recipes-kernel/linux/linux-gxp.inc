DESCRIPTION = "Linux kernel for GXP"
SECTION = "kernel"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=6bc538ed5bd9a7fc9398086aedcd7e46"

PROVIDES += "virtual/kernel"

FILESEXTRAPATHS:prepend:linux-gxp := "${THISDIR}/linux-gxp:"
FILESEXTRAPATHS:prepend := "${THISDIR}/linux-gxp-${LINUX_VERSION}:"

inherit kernel
require recipes-kernel/linux/linux-yocto.inc

ERROR_QA:remove = "buildpaths"
ERROR_QA:remove = "patch-status"
WARN_QA:append = " buildpaths"
WARN_QA:append = " patch-status"

KERNEL_VERSION_SANITY_SKIP = "1"
LINUX_VERSION_EXTENSION ?= "-${SRCREV}"
PV = "${LINUX_VERSION}+git${SRCPV}"

KSRC ?= "git://github.com/HewlettPackard/gxp-linux.git;branch=${KBRANCH};protocol=https"

KCONFIG_MODE = "--alldefconfig"

SRC_URI = "${KSRC}"
SRC_URI += " file://defconfig"
SRC_URI += " file://gxp.dts"

require linux-gxp-multi-dtb.inc
