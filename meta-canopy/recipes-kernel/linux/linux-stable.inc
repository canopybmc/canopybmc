DESCRIPTION = "Linux stable kernel"
SECTION = "kernel"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=6bc538ed5bd9a7fc9398086aedcd7e46"

PROVIDES += "virtual/kernel"

inherit kernel
require recipes-kernel/linux/linux-yocto.inc

LINUX_VERSION_EXTENSION ?= "-${SRCREV}"
PV = "${LINUX_VERSION}+git${SRCPV}"

KSRC ?= "git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git;protocol=https;branch=${KBRANCH};tag=v${LINUX_VERSION}"
SRC_URI = "${KSRC}"

