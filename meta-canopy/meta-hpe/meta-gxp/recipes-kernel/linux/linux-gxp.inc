DESCRIPTION = "Linux kernel for GXP"
SECTION = "kernel"
LICENSE = "GPL-2.0-only"

PROVIDES += "virtual/kernel"

KCONFIG_MODE = "--alldefconfig"

KSRC ?= "git://github.com/HewlettPackard/gxp-linux.git;branch=${KBRANCH};protocol=https"
SRC_URI = "${KSRC}"
SRC_URI += " file://defconfig"
SRC_URI += " file://gxp.dts"
SRC_URI += " file://0001-arm-gxp-add-device-tree-overlay-support-via-configfs.patch"
SRC_URI += " file://0002-arm-gxp-add-HPE-GXP-SoC-platform-support.patch"
SRC_URI += " file://0003-arm-gxp-add-GXP-peripheral-drivers.patch"
SRC_URI += " file://0004-arm-gxp-add-KCS-and-SSIF-IPMI-BMC-drivers.patch"
SRC_URI += " file://0005-arm-gxp-add-hardware-monitoring-drivers.patch"
SRC_URI += " file://0006-arm-gxp-add-PECI-bus-driver-and-MFD-updates.patch"
SRC_URI += " file://0007-arm-gxp-add-thumbnail-video-driver.patch"
SRC_URI += " file://0008-arm-gxp-add-miscellaneous-platform-support.patch"

# Device tree overlays for HPE ProLiant G11 server variants
GXP_DT_OVERLAYS = " \
    proliant_0x0235 \
    proliant_0x0236 \
    proliant_0x0241 \
    proliant_0x0243 \
    proliant_0x0244 \
    proliant_0x0245 \
    proliant_0x0246 \
    proliant_0x0248 \
    proliant_0x0249 \
    proliant_0x024a \
    proliant_0x0250 \
    proliant_0x0261 \
    proliant_0x0263 \
    proliant_0x0264 \
    proliant_0x0271 \
    proliant_0x0273 \
"

SRC_URI += "${@' '.join(['file://%s.dtso' % o for o in d.getVar('GXP_DT_OVERLAYS').split()])}"

FILESEXTRAPATHS:prepend:linux-gxp := "${THISDIR}/linux-gxp:"

LINUX_VERSION_EXTENSION ?= "-${SRCREV}"

PV = "${LINUX_VERSION}+git${SRCPV}"

inherit kernel
require recipes-kernel/linux/linux-yocto.inc

ERROR_QA:remove = "buildpaths"
WARN_QA:append = " buildpaths"

# From 4.16+ the COPYING file changed
LIC_FILES_CHKSUM = "file://COPYING;md5=6bc538ed5bd9a7fc9398086aedcd7e46"
KERNEL_FEATURES:remove = "phosphor-gpio-keys"

# Enable overlay symbol generation for base DTB
DEVICETREE_FLAGS += "-@"

# Copy base device tree source to kernel tree
do_patch:append() {
    if [ -f "${UNPACKDIR}/gxp.dts" ]; then
        cp ${UNPACKDIR}/gxp.dts ${STAGING_KERNEL_DIR}/arch/${ARCH}/boot/dts/
    fi
}

# Compile device tree overlays manually after kernel build
do_compile:append() {
    # Get the DTC compiler from the kernel build
    DTC="${B}/scripts/dtc/dtc"

    # Compile each overlay (files are in UNPACKDIR for kernel recipes)
    for overlay in ${GXP_DT_OVERLAYS}; do
        if [ -f "${UNPACKDIR}/${overlay}.dtso" ]; then
            ${DTC} -@ -I dts -O dtb \
                -o ${B}/arch/${ARCH}/boot/dts/${overlay}.dtbo \
                ${UNPACKDIR}/${overlay}.dtso
        fi
    done
}

# Install overlays to /boot
do_install:append() {
    install -d ${D}/boot
    for overlay in ${GXP_DT_OVERLAYS}; do
        if [ -f "${B}/arch/${ARCH}/boot/dts/${overlay}.dtbo" ]; then
            install -m 0644 ${B}/arch/${ARCH}/boot/dts/${overlay}.dtbo ${D}/boot/
        fi
    done
}

# Package overlays
FILES:${PN} += "/boot/*.dtbo"
