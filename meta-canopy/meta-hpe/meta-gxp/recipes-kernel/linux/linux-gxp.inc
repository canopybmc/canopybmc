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

# Server IDs for HPE ProLiant G11 variants
GXP_SERVER_IDS = " \
    0x0235 \
    0x0236 \
    0x0241 \
    0x0243 \
    0x0244 \
    0x0245 \
    0x0246 \
    0x0248 \
    0x0249 \
    0x024a \
    0x0250 \
    0x0261 \
    0x0263 \
    0x0264 \
    0x0271 \
    0x0273 \
"

# Device tree overlays generated from server IDs
GXP_DT_OVERLAYS = "${@' '.join(['gxp-%s' % s for s in d.getVar('GXP_SERVER_IDS').split()])}"

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

EXTERNAL_KERNEL_DEVICETREE = "${B}/gxp-overlays"
GXP_BASE_DTB = "gxp.dtb"
FIT_CONF_DEFAULT_DTB = "${GXP_BASE_DTB}"
UBOOT_DTB_LOADADDRESS = "0x48000000"
UBOOT_DTBO_LOADADDRESS = "0x48100000"

# Compile device tree overlays and stage for FIT inclusion
do_compile:append() {
    DTC="${B}/scripts/dtc/dtc"

    for overlay in ${GXP_DT_OVERLAYS}; do
        if [ -f "${UNPACKDIR}/${overlay}.dtso" ]; then
            ${DTC} -@ -I dts -O dtb \
                -o ${B}/arch/${ARCH}/boot/dts/${overlay}.dtbo \
                ${UNPACKDIR}/${overlay}.dtso
        fi
    done

    install -d ${B}/gxp-overlays
    for overlay in ${GXP_DT_OVERLAYS}; do
        if [ -f "${B}/arch/${ARCH}/boot/dts/${overlay}.dtbo" ]; then
            install -m 0644 ${B}/arch/${ARCH}/boot/dts/${overlay}.dtbo ${B}/gxp-overlays/
        fi
    done
}

# Add FIT configurations combining base DTB with server-specific overlays.
# U-Boot selects configuration via board_fit_config_name_match() based on server_id.
python gxp_fitimage_add_overlay_configs() {
    import os
    import re
    import subprocess

    b = d.getVar('B')
    kernel_output_dir = d.getVar('KERNEL_OUTPUT_DIR')
    overlays = d.getVar('GXP_DT_OVERLAYS').split()
    base_dtb = d.getVar('GXP_BASE_DTB')
    fit_hash_alg = d.getVar('FIT_HASH_ALG') or 'sha256'
    fit_conf_prefix = d.getVar('FIT_CONF_PREFIX') or 'conf-'
    uboot_mkimage = d.getVar('UBOOT_MKIMAGE')
    uboot_mkimage_dtcopts = d.getVar('UBOOT_MKIMAGE_DTCOPTS') or ''

    its_files = [os.path.join(b, f) for f in os.listdir(b)
                 if f.startswith('fit-image') and f.endswith('.its')]

    if not its_files:
        bb.debug(1, "No FIT ITS files found")
        return

    for its_file in its_files:
        with open(its_file, 'r') as f:
            content = f.read()

        if 'ramdisk-1' not in content:
            bb.debug(1, f"Skipping {its_file} - no ramdisk")
            continue

        new_configs = []
        for overlay in overlays:
            overlay_fdt_name = f"fdt-{overlay}.dtbo"
            if overlay_fdt_name not in content:
                bb.debug(1, f"Overlay {overlay}.dtbo not in FIT")
                continue

            config = f'''
                {fit_conf_prefix}{overlay} {{
                        description = "{overlay}";
                        kernel = "kernel-1";
                        fdt = "fdt-{base_dtb}", "{overlay_fdt_name}";
                        ramdisk = "ramdisk-1";
                        hash-1 {{
                                algo = "{fit_hash_alg}";
                        }};
                }};'''
            new_configs.append(config)

        if not new_configs:
            bb.debug(1, f"No overlay configs to add to {its_file}")
            continue

        configs_end = re.search(r'(\n[\t ]*\};[\t ]*\n)([\t ]*\};[\s]*\};[\s]*)$', content, re.DOTALL)
        if not configs_end:
            bb.warn(f"Could not find configurations section end in {its_file}")
            continue

        insert_pos = configs_end.start(2)
        new_content = content[:insert_pos] + '\n'.join(new_configs) + '\n\t' + content[insert_pos:]

        with open(its_file, 'w') as f:
            f.write(new_content)

        bb.note(f"Added {len(new_configs)} overlay configurations to {its_file}")

        fitimage_name = os.path.basename(its_file).replace('fit-image', 'fitImage').replace('.its', '')
        fitimage_path = os.path.join(b, kernel_output_dir, fitimage_name)

        mkimage_cmd = [uboot_mkimage]
        if uboot_mkimage_dtcopts:
            mkimage_cmd.extend(['-D', uboot_mkimage_dtcopts])
        mkimage_cmd.extend(['-f', its_file, fitimage_path])

        result = subprocess.run(mkimage_cmd, cwd=b, capture_output=True, text=True)
        if result.returncode != 0:
            bb.fatal(f"mkimage failed: {result.stderr}")
}

do_assemble_fitimage[postfuncs] += "gxp_fitimage_add_overlay_configs"
do_assemble_fitimage_initramfs[postfuncs] += "gxp_fitimage_add_overlay_configs"
