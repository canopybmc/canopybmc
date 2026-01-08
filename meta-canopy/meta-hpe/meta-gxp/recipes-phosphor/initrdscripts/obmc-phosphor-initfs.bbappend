# Add GXP device tree overlay loading to init script
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://gxp-obmc-init.sh"

do_install:append() {
        # Replace init with GXP version that includes DT overlay loading
        install -m 0755 ${UNPACKDIR}/gxp-obmc-init.sh ${D}/init
}
