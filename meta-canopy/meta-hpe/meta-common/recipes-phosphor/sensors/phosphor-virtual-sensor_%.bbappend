FILESEXTRAPATHS:prepend := "${THISDIR}/phosphor-virtual-sensor:"

SRC_URI += "file://virtual_sensor_config.json"

do_install:append() {
    install -d ${D}${datadir}/phosphor-virtual-sensor
    install -m 0644 ${UNPACKDIR}/virtual_sensor_config.json ${D}${datadir}/phosphor-virtual-sensor/virtual_sensor_config.json
}
