FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://10-sbin-to-bin.conf \
    "

FILES:${PN}:append = " \
    ${systemd_system_unitdir}/clear-once.service.d/ \
    "

do_install:append() {
    install -d ${D}${systemd_system_unitdir}/clear-once.service.d/
    install -m 0644 ${UNPACKDIR}/10-sbin-to-bin.conf \
        ${D}${systemd_system_unitdir}/clear-once.service.d/
}
