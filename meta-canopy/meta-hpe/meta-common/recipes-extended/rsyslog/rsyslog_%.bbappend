FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

PACKAGECONFIG:append = " imjournal"

SRC_URI += "file://redfish-eventlog.conf"

do_install:append() {
    install -m 0644 ${UNPACKDIR}/redfish-eventlog.conf \
        ${D}${sysconfdir}/rsyslog.d/redfish-eventlog.conf
}
