# linux-gxp-multi-dtb.inc - Multi-DTB FIT image support for HPE GXP platforms
#
# Adds device tree overlay support for FIT images. U-Boot selects the
# appropriate configuration at runtime based on server_id from hardware.

GXP_OVERLAY_DIR = "${B}/gxp-overlays"
EXTERNAL_KERNEL_DEVICETREE = "${GXP_OVERLAY_DIR}"
GXP_BASE_DTB = "${KERNEL_DEVICETREE}"
GXP_BASE_DTS = "${@d.getVar('KERNEL_DEVICETREE').replace('.dtb', '.dts')}"
FIT_CONF_DEFAULT_DTB = "${GXP_BASE_DTB}"

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
GXP_DT_OVERLAYS = "${@' '.join(['gxp-%s' % s for s in d.getVar('GXP_SERVER_IDS').split()])}"
SRC_URI += "${@' '.join(['file://%s.dtso' % o for o in d.getVar('GXP_DT_OVERLAYS').split()])}"

# Enable overlay symbol generation for base DTB
KERNEL_DTC_FLAGS = "-@"

# Copy base device tree source to kernel tree
do_patch:append() {
    if [ -f "${UNPACKDIR}/${GXP_BASE_DTS}" ]; then
        cp "${UNPACKDIR}/${GXP_BASE_DTS}" "${STAGING_KERNEL_DIR}/arch/${ARCH}/boot/dts/"
    fi
}

# Compile device tree overlays directly to staging directory
do_compile:append() {
    DTC="${B}/scripts/dtc/dtc"
    install -d "${GXP_OVERLAY_DIR}"

    for overlay in ${GXP_DT_OVERLAYS}; do
        if [ -f "${UNPACKDIR}/${overlay}.dtso" ]; then
            ${DTC} -@ -I dts -O dtb \
                -o "${GXP_OVERLAY_DIR}/${overlay}.dtbo" \
                "${UNPACKDIR}/${overlay}.dtso"
        fi
    done
}

# Add FIT configurations combining base DTB with server-specific overlays.
# U-Boot selects configuration via board_fit_config_name_match() based on server_id.
python gxp_fitimage_add_overlay_configs_initramfs() {
    import os
    import re
    import subprocess

    initramfs_image = d.getVar('INITRAMFS_IMAGE')
    if not initramfs_image:
        return

    b = d.getVar('B')
    kernel_output_dir = d.getVar('KERNEL_OUTPUT_DIR')
    overlays = d.getVar('GXP_DT_OVERLAYS').split()
    base_dtb = d.getVar('GXP_BASE_DTB')
    fit_hash_alg = d.getVar('FIT_HASH_ALG') or 'sha256'
    fit_conf_prefix = d.getVar('FIT_CONF_PREFIX') or 'conf-'
    uboot_mkimage = d.getVar('UBOOT_MKIMAGE')
    uboot_mkimage_dtcopts = d.getVar('UBOOT_MKIMAGE_DTCOPTS') or ''

    its_file = os.path.join(b, f'fit-image-{initramfs_image}.its')
    if not os.path.exists(its_file):
        bb.debug(1, f"ITS file not found: {its_file}")
        return

    with open(its_file, 'r') as f:
        content = f.read()

    # kernel-fitimage.bbclass converts slashes to underscores in FDT section names
    base_dtb_sect = base_dtb.replace('/', '_')

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
                        fdt = "fdt-{base_dtb_sect}", "{overlay_fdt_name}";
                        ramdisk = "ramdisk-1";
                        hash-1 {{
                                algo = "{fit_hash_alg}";
                        }};
                }};'''
        new_configs.append(config)

    if not new_configs:
        bb.debug(1, f"No overlay configs to add to {its_file}")
        return

    configs_end = re.search(r'(\n[\t ]*\};[\t ]*\n)([\t ]*\};[\s]*\};[\s]*)$', content, re.DOTALL)
    if not configs_end:
        bb.warn(f"Could not find configurations section end in {its_file}")
        return

    insert_pos = configs_end.start(2)
    new_content = content[:insert_pos] + '\n'.join(new_configs) + '\n\t' + content[insert_pos:]

    with open(its_file, 'w') as f:
        f.write(new_content)

    bb.note(f"Added {len(new_configs)} overlay configurations to {its_file}")

    fitimage_name = f'fitImage-{initramfs_image}'
    fitimage_path = os.path.join(b, kernel_output_dir, fitimage_name)

    mkimage_cmd = [uboot_mkimage]
    if uboot_mkimage_dtcopts:
        mkimage_cmd.extend(['-D', uboot_mkimage_dtcopts])
    mkimage_cmd.extend(['-f', its_file, fitimage_path])

    result = subprocess.run(mkimage_cmd, cwd=b, capture_output=True, text=True)
    if result.returncode != 0:
        bb.fatal(f"mkimage failed: {result.stderr}")
}

do_assemble_fitimage_initramfs[postfuncs] += "gxp_fitimage_add_overlay_configs_initramfs"
