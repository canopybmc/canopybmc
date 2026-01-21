FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://fw_env.config"

do_install:append() {
    install -d ${D}${sysconfdir}
    install -m 644 ${UNPACKDIR}/fw_env.config ${D}${sysconfdir}/fw_env.config
}

RDEPENDS:${PN} += "udev-gxp-mtd-partitions"
