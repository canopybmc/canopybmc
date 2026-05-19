FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

PACKAGECONFIG:append = " json"
PACKAGECONFIG:remove = "sensor-monitor"
SYSTEMD_SERVICE:${PN}-sensor-monitor:remove = "sensor-monitor.service"

SRC_URI += " \
        file://presence.json \
        "

do_configure:prepend() {
    mkdir -p ${S}/presence/config_files/${MACHINE}
    install -D ${UNPACKDIR}/presence.json ${S}/presence/config_files/${MACHINE}/config.json
}
