FILESEXTRAPATHS:append := "${THISDIR}/${PN}:"

PACKAGECONFIG:append = " json"
PACKAGECONFIG:remove = "sensor-monitor"
SYSTEMD_SERVICE:${PN}-sensor-monitor:remove = "sensor-monitor.service"
SYSTEMD_LINK:${PN}-sensor-monitor = ""

SRC_URI:append = " file://presence.json"

do_configure:prepend() {
        mkdir -p ${S}/presence/config_files/${MACHINE}
        cp ${UNPACKDIR}/presence.json ${S}/presence/config_files/${MACHINE}/config.json
}
