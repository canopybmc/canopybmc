# linux-gxp-multi-dtb.inc - Multi-DTB FIT image support for HPE GXP platforms
#
# Adds device tree overlay support for FIT images. U-Boot selects the
# appropriate configuration at runtime based on server_id from hardware.

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

# Enable overlay symbol generation for base DTB
KERNEL_DTC_FLAGS = "-@"

# Copy base device tree source to kernel tree
do_patch:append() {
    if [ -f "${UNPACKDIR}/${GXP_BASE_DTS}" ]; then
        cp "${UNPACKDIR}/${GXP_BASE_DTS}" "${STAGING_KERNEL_DIR}/arch/${ARCH}/boot/dts/"
    fi
}
