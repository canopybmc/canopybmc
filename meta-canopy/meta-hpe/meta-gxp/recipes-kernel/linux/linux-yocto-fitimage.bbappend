FILESEXTRAPATHS:prepend := "${THISDIR}/linux-stable:${THISDIR}/../../recipes-bsp/images/gxp-section:"

# FIT image signing — same key override pattern as gxp-uboot-sig.bb.
# Override HPE_SIGNING_KEY in local.conf for production builds.
HPE_OSFCI_SIGNING_KEY = "hpe_osfci_private_key.pem"
HPE_SIGNING_KEY ?= "${HPE_OSFCI_SIGNING_KEY}"

# In PKCS#11 mode HPE_SIGNING_KEY is a token URI (pkcs11:...), not a file.
SRC_URI += "${@'' if (d.getVar('HPE_SIGNING_KEY') or '').startswith('pkcs11:') else 'file://${HPE_SIGNING_KEY}'}"

# mkimage needs <keyname>.key + <keyname>.crt in a single directory.
# The cert is self-signed and generated at build time from the private key;
# U-Boot only uses it to extract the public key for embedding in its DTB.
FIT_KERNEL_SIGN_KEYDIR  = "${B}/hpe-fit-keys"
FIT_KERNEL_SIGN_KEYNAME = "hpe_fit"

do_prepare_fit_keys() {
    install -d "${B}/hpe-fit-keys"

    case "${HPE_SIGNING_KEY}" in
        pkcs11:*)
            openssl req -new -x509 -days 36500 -subj "/" \
                -engine pkcs11 -keyform engine \
                -key  "${HPE_SIGNING_KEY}" \
                -out  "${B}/hpe-fit-keys/hpe_fit.crt"
            ;;
        *)
            # PEM mode (usually the default).
            install -m 0600 "${UNPACKDIR}/${HPE_SIGNING_KEY}" "${B}/hpe-fit-keys/hpe_fit.key"
            openssl req -new -x509 -days 36500 -subj "/" \
                -key "${B}/hpe-fit-keys/hpe_fit.key" \
                -out "${B}/hpe-fit-keys/hpe_fit.crt"
            ;;
    esac
}
addtask do_prepare_fit_keys after do_unpack before do_compile
do_prepare_fit_keys[depends] += "openssl-native:do_populate_sysroot"

# Mirror the overlay list from linux-gxp-multi-dtb.inc so this recipe knows
# which .dtso files to compile.
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

GXP_OVERLAY_DIR = "${B}/gxp-overlays"
EXTERNAL_KERNEL_DEVICETREE = "${GXP_OVERLAY_DIR}"
FIT_CONF_DEFAULT_DTB = "${@os.path.basename(d.getVar('KERNEL_DEVICETREE'))}"

# Compile .dtso overlays into the staging directory before do_compile.
# Needs to be a dedicated task as mixing shell and Python in the same task
# doesn't work.
do_compile_gxp_overlays() {
    install -d "${GXP_OVERLAY_DIR}"
    for overlay in ${GXP_DT_OVERLAYS}; do
        src="${UNPACKDIR}/${overlay}.dtso"
        out="${GXP_OVERLAY_DIR}/${overlay}.dtbo"
        if [ -f "${src}" ]; then
            "${STAGING_BINDIR_NATIVE}/dtc" -@ -I dts -O dtb -o "${out}" "${src}"
        else
            bbwarn "GXP overlay source not found: ${src}"
        fi
    done
}
addtask do_compile_gxp_overlays after do_unpack before do_compile
do_compile_gxp_overlays[depends] += "dtc-native:do_populate_sysroot"

# Rewrite the ITS to replace those stub configs with multi-fdt configs
python do_compile:append() {
    def _gxp_rewrite_fitimage(d):
        import os
        import re
        import subprocess
        import shlex

        b               = d.getVar('B')
        overlays        = d.getVar('GXP_DT_OVERLAYS').split()
        # GXP_BASE_DTB is defined in linux-gxp-multi-dtb.inc (kernel recipe
        # context only). KERNEL_DEVICETREE is a machine variable available
        # everywhere and resolves to the same value.
        base_dtb        = d.getVar('KERNEL_DEVICETREE')
        fit_hash_alg    = d.getVar('FIT_HASH_ALG') or 'sha256'
        fit_sign_alg    = d.getVar('FIT_SIGN_ALG') or 'rsa2048'
        fit_pad_alg     = d.getVar('FIT_PAD_ALG') or 'pkcs-1.5'
        fit_conf_prefix = d.getVar('FIT_CONF_PREFIX') or 'conf-'
        uboot_mkimage   = d.getVar('UBOOT_MKIMAGE')
        uboot_mkimage_dtcopts = d.getVar('UBOOT_MKIMAGE_DTCOPTS') or ''
        uboot_mkimage_sign    = d.getVar('UBOOT_MKIMAGE_SIGN') or uboot_mkimage
        uboot_mkimage_sign_args = d.getVar('UBOOT_MKIMAGE_SIGN_ARGS') or ''
        sign_enable     = (d.getVar('FIT_KERNEL_SIGN_ENABLE') or '0') == '1'
        sign_keydir     = d.getVar('FIT_KERNEL_SIGN_KEYDIR') or ''
        sign_keyname    = d.getVar('FIT_KERNEL_SIGN_KEYNAME') or ''
        hpe_signing_key = d.getVar('HPE_SIGNING_KEY') or ''
        pkcs11_mode     = hpe_signing_key.startswith('pkcs11:')

        its_file    = os.path.join(b, 'fit-image.its')
        fitimg_path = os.path.join(b, 'fitImage')

        if not os.path.exists(its_file):
            bb.fatal(f"GXP DTB append: fit-image.its not found at {its_file}")

        with open(its_file, 'r') as f:
            content = f.read()

        # kernel-fit-image.bbclass names fdt nodes as "fdt-<filename>" where
        # slashes in the filename are replaced with underscores.
        base_dtb_node = "fdt-" + os.path.basename(base_dtb)

        # Verify the base DTB node is present in the ITS.
        if base_dtb_node not in content:
            bb.fatal(f"GXP DTB append: base DTB node '{base_dtb_node}' not found in {its_file}")

        found_overlays = []
        for overlay in overlays:
            overlay_node = "fdt-%s.dtbo" % overlay
            if overlay_node not in content:
                bb.debug(1, f"GXP DTB append: overlay node '{overlay_node}' not in ITS, skipping")
                continue
            found_overlays.append((overlay, overlay_node))

        if not found_overlays:
            bb.warn("GXP DTB append: no overlay fdt nodes found in ITS, skipping config rewrite")
            return

        # Each config references both the base DTB and one server-specific
        # overlay.
        # U-Boot then selects the configuration via
        # board_fit_config_name_match() based on server_id.
        new_configs = []
        for overlay, overlay_node in found_overlays:
            sign_node = ""
            if sign_enable:
                sign_node = """
                            signature-1 {{
                                    algo = "{hash},{sign}";
                                    key-name-hint = "{keyname}";
                                    padding = "{pad}";
                                    sign-images = "kernel", "fdt", "ramdisk";
                            }};""".format(
                    hash=fit_hash_alg, sign=fit_sign_alg,
                    keyname=sign_keyname, pad=fit_pad_alg)

            config = '''
                    {prefix}{overlay} {{
                            description = "{overlay}";
                            kernel = "kernel-1";
                            fdt = "{base}", "{overlay_node}";
                            ramdisk = "ramdisk-1";
                            hash-1 {{
                                    algo = "{hash}";
                            }};{sign}
                    }};'''.format(
                prefix=fit_conf_prefix,
                overlay=overlay,
                base=base_dtb_node,
                overlay_node=overlay_node,
                hash=fit_hash_alg,
                sign=sign_node,
            )
            new_configs.append(config)

        for overlay, overlay_node in found_overlays:
            node_name = fit_conf_prefix + overlay + '.dtbo'
            start_match = re.search(r'\n[ \t]*' + re.escape(node_name) + r'[ \t]*\{', content)
            if not start_match:
                continue
            depth = 0
            end = start_match.start()
            for i in range(start_match.start(), len(content)):
                if content[i] == '{':
                    depth += 1
                elif content[i] == '}':
                    depth -= 1
                    if depth == 0:
                        end = i + 1
                        while end < len(content) and content[end] in (' ', '\t', ';'):
                            end += 1
                        break
            content = content[:start_match.start()] + content[end:]

        configs_end = re.search(r'(\n[\t ]*\};[\t ]*\n)([\t ]*\};[\s]*\};[\s]*)$', content, re.DOTALL)
        if not configs_end:
            bb.fatal(f"GXP DTB append: could not locate configurations section end in {its_file}")

        insert_pos = configs_end.start(2)
        new_content = content[:insert_pos] + '\n'.join(new_configs) + '\n\t' + content[insert_pos:]

        with open(its_file, 'w') as f:
            f.write(new_content)

        bb.note(f"GXP DTB append: rewrote {len(new_configs)} overlay configs in {its_file}")

        mkimage_cmd = [uboot_mkimage]
        if uboot_mkimage_dtcopts:
            mkimage_cmd += ['-D', uboot_mkimage_dtcopts]
        mkimage_cmd += ['-f', its_file, fitimg_path]

        result = subprocess.run(mkimage_cmd, cwd=b, capture_output=True, text=True)
        if result.returncode != 0:
            bb.fatal(f"GXP DTB append: mkimage re-assembly failed:\n{result.stderr}")

        bb.note("GXP DTB append: re-assembled fitImage")

        if sign_enable:
            sign_key_path = os.path.join(sign_keydir, sign_keyname)
            if not pkcs11_mode and not os.path.exists(sign_key_path + '.key'):
                bb.fatal(f"GXP DTB append: sign key {sign_key_path}.key not found")
            if not os.path.exists(sign_key_path + '.crt'):
                bb.fatal(f"GXP DTB append: sign cert {sign_key_path}.crt not found")

            sign_cmd = [uboot_mkimage_sign, '-F', '-k', sign_keydir, '-r', fitimg_path]
            if pkcs11_mode:
                sign_cmd += ['-N', 'pkcs11', '-G', hpe_signing_key]
            if uboot_mkimage_dtcopts:
                sign_cmd += ['-D', uboot_mkimage_dtcopts]
            if uboot_mkimage_sign_args:
                sign_cmd += shlex.split(uboot_mkimage_sign_args)

            result = subprocess.run(sign_cmd, cwd=b, capture_output=True, text=True)
            if result.returncode != 0:
                bb.fatal(f"GXP DTB append: mkimage re-sign failed:\n{result.stderr}")

            bb.note("GXP DTB append: re-signed fitImage")

    _gxp_rewrite_fitimage(d)
}
