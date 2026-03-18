FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
ERROR_QA:remove = "patch-status"

SRC_URI:append = " \
        file://0001-dbus-support-PWM-only-fan-sensors-in-PID-configurati.patch \
        file://0002-main-add-configurable-zone-rebuild-retry-delay.patch \
        file://0003-dbus-skip-zone-rebuild-on-sensor-removal.patch \
        file://10-after-entity-manager.conf \
        file://20-retry-delay.conf \
        "

FILES:${PN}:append = " \
        ${systemd_system_unitdir}/phosphor-pid-control.service.d/ \
        "

do_install:append() {
    install -d ${D}${systemd_system_unitdir}/phosphor-pid-control.service.d
    install -m 0644 ${UNPACKDIR}/10-after-entity-manager.conf \
        ${D}${systemd_system_unitdir}/phosphor-pid-control.service.d/
    install -m 0644 ${UNPACKDIR}/20-retry-delay.conf \
        ${D}${systemd_system_unitdir}/phosphor-pid-control.service.d/
}
