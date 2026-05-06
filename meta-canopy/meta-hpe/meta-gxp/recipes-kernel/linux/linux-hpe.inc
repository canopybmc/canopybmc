DESCRIPTION = "Linux kernel for HPE GXP"
SECTION = "kernel"
LICENSE = "GPL-2.0-only"

PROVIDES += "virtual/kernel"

KCONFIG_MODE = "--alldefconfig"

KSRC ?= "git://github.com/openbmc/linux;protocol=https;branch=${KBRANCH}"
SRC_URI += "${KSRC}"

LINUX_VERSION_EXTENSION ?= "-${SRCREV}"

PV = "${LINUX_VERSION}+git${SRCPV}"

inherit kernel kernel-fit-extra-artifacts
require recipes-kernel/linux/linux-yocto.inc

ERROR_QA:remove = "buildpaths"
WARN_QA:append = "buildpaths"

# From 5.6+ the COPYING file changed
LIC_FILES_CHKSUM = "file://COPYING;md5=6bc538ed5bd9a7fc9398086aedcd7e46"
