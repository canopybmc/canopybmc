FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-virtualSensor-make-exprtk-support-optional.patch"

# exprtk costs ~1MB of text and is only needed for Expression-based sensors in
# the static JSON config. Canopy configures virtual sensors through
# entity-manager, which uses the D-Bus calculation types instead.
PACKAGECONFIG ??= ""
PACKAGECONFIG[exprtk] = "-Dexprtk=enabled,-Dexprtk=disabled,exprtk"

DEPENDS:remove = "exprtk"

# The daemon only looks for entity-manager configs when the static config holds
# an entry with Desc.Config set to D-Bus. Replace the upstream sample, which
# carries an Expression that cannot be evaluated without exprtk.
SRC_URI += "file://virtual_sensor_config.json"

do_install:append() {
    install -D -m 0644 ${UNPACKDIR}/virtual_sensor_config.json \
        ${D}${datadir}/phosphor-virtual-sensor/virtual_sensor_config.json
}
