FILESEXTRAPATHS:prepend := "${THISDIR}/phosphor-gpio-monitor:"

SRC_URI += " \
    file://phosphor-multi-gpio-monitor.json \
    file://uid-button-toggle.sh \
    file://uid-button-toggle.service \
    "

SYSTEMD_LINK:${PN}-monitor += " \
    ../phosphor-multi-gpio-monitor.service:multi-user.target.requires/phosphor-multi-gpio-monitor.service \
    "

FILES:${PN}-monitor += "${bindir}/uid-button-toggle.sh ${systemd_system_unitdir}/uid-button-toggle.service"

do_install:append() {
    install -d ${D}${datadir}/${BPN}
    install -m 0644 ${UNPACKDIR}/phosphor-multi-gpio-monitor.json ${D}${datadir}/${BPN}/phosphor-multi-gpio-monitor.json

    install -d ${D}${bindir}
    install -m 0755 ${UNPACKDIR}/uid-button-toggle.sh ${D}${bindir}/uid-button-toggle.sh

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/uid-button-toggle.service ${D}${systemd_system_unitdir}/uid-button-toggle.service
}
