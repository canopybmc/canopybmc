SUMMARY = "BMC health checks backing the hardware watchdog"
DESCRIPTION = "Runs a local ssh login attempt and any checks dropped into \
/etc/bmc-health.d. A failing BMC stops petting its systemd service watchdog, \
which escalates to a BMC reboot."

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

RDEPENDS:${PN} = "openssh-ssh"

SRC_URI = " \
    file://bmc-health \
    file://bmc-health.service \
    file://ssh \
    "

S = "${UNPACKDIR}"

inherit systemd

SYSTEMD_SERVICE:${PN} = "bmc-health.service"

do_install() {
    install -d ${D}${sbindir}
    install -m 0755 ${S}/bmc-health ${D}${sbindir}/

    install -d ${D}${libexecdir}/bmc-health
    install -m 0755 ${S}/ssh ${D}${libexecdir}/bmc-health/

    install -d ${D}${sysconfdir}/bmc-health.d

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${S}/bmc-health.service ${D}${systemd_system_unitdir}/
}
