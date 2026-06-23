FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://obmc-flash-host-bios@.service \
    file://obmc-flash-host-bios \
"

PACKAGECONFIG:append = " flash_bios"
RDEPENDS:${PN}-updater:append = " bash mtd-utils"

# Without this GXP SROT would fail if u-boot changes after a non-all tarball
# update because the old RSA signature is left on flash while u-boot.bin
# is replaced.
EXTRA_OEMESON:append = " -Doptional-images='image-uboot-sig'"

do_install:append() {
    install -d ${D}${sbindir}
    install -m 0755 ${UNPACKDIR}/obmc-flash-host-bios \
        ${D}${sbindir}/obmc-flash-host-bios

    install -m 0644 ${UNPACKDIR}/obmc-flash-host-bios@.service \
        ${D}${systemd_system_unitdir}/obmc-flash-host-bios@.service
}

FILES:${PN}-bios += " \
    ${sbindir}/obmc-flash-host-bios \
    ${systemd_system_unitdir}/obmc-flash-host-bios@.service \
"
