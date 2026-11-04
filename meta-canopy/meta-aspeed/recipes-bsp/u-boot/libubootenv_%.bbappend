# SPI NOR parts of the libubootenv append in openbmc change 81057.
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " file://fw_env.config"

do_install:append() {
    install -d ${D}${sysconfdir}
    install -m 644 ${UNPACKDIR}/fw_env.config ${D}${sysconfdir}/fw_env.config

    # meta-phosphor hardcodes /sbin/fw_printenv and /sbin/fw_setenv
    # (clear-once.service, phosphor-static-norootfs-init).
    install -d ${D}${base_sbindir}
    ln -sr ${D}${bindir}/fw_printenv ${D}${base_sbindir}/fw_printenv
    ln -sr ${D}${bindir}/fw_setenv ${D}${base_sbindir}/fw_setenv
}

FILES:${PN}-bin:append = " \
    ${base_sbindir}/fw_printenv \
    ${base_sbindir}/fw_setenv \
"

# Fall back to U-Boot's default environment when the flash copy is blank.
RDEPENDS:${PN}-bin:append = " u-boot-aspeed-env"

RDEPENDS:${PN}:append = " ${PN}-bin"

# fw_env.config is machine specific
PACKAGE_ARCH = "${MACHINE_ARCH}"
