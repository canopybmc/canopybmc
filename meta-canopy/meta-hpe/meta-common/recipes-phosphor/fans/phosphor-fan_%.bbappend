FILESEXTRAPATHS:append := "${THISDIR}/${PN}:"

PACKAGECONFIG:append = " json"
PACKAGECONFIG:remove = "sensor-monitor"
SYSTEMD_SERVICE:${PN}-sensor-monitor:remove = "sensor-monitor.service"
SYSTEMD_LINK:${PN}-sensor-monitor = ""

SRC_URI:append = " \
        file://10-after-inv-manager.conf \
        file://presence.json \
        "

FILES:${PN}-presence-tach:append = " \
        ${systemd_system_unitdir}/phosphor-fan-presence-tach@.service.d/ \
        "

do_configure:prepend() {
    mkdir -p ${S}/presence/config_files/${MACHINE}
    cp ${UNPACKDIR}/presence.json ${S}/presence/config_files/${MACHINE}/config.json
}

do_install:append() {
    install -d ${D}${systemd_system_unitdir}/phosphor-fan-presence-tach@.service.d/
    install -m 0644 ${UNPACKDIR}/10-after-inv-manager.conf \
        ${D}${systemd_system_unitdir}/phosphor-fan-presence-tach@.service.d/
}
