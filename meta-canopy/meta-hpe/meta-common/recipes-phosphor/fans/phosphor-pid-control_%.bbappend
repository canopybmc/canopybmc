FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
SRC_URI += "file://0001-dbus-support-PWM-only-fan-sensors-in-PID-configurati.patch"
SRC_URI += "file://0002-main-add-configurable-zone-rebuild-retry-delay.patch"
SRC_URI += "file://0003-dbus-skip-zone-rebuild-on-sensor-removal.patch"
SRC_URI += "file://10-retry-delay.conf"
ERROR_QA:remove = "patch-status"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}/phosphor-pid-control.service.d
    install -m 0644 ${UNPACKDIR}/10-retry-delay.conf \
        ${D}${systemd_system_unitdir}/phosphor-pid-control.service.d/
}

FILES:${PN} += "${systemd_system_unitdir}/phosphor-pid-control.service.d/"
