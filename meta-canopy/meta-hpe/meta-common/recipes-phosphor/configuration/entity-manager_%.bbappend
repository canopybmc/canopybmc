FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# Enable devicetree VPD parser for platform identification
PACKAGECONFIG:append = " dts-vpd"

SRC_URI += " \
    file://blacklist.json \
    file://amd_cpu.json \
    file://dl110-g11.json \
    file://dl145-g11.json \
    file://dl320-g11.json \
    file://dl325-g11.json \
    file://dl345-g11.json \
    file://dl360-g11.json \
    file://dl365-g11.json \
    file://dl380-g11.json \
    file://dl380a-g11.json \
    file://dl385-g11.json \
    file://dl560-g11.json \
    file://hpe_drv.json \
    file://hpe_psu.json \
    file://rl300-g11.json \
"

do_install:append() {
    install -d ${D}${datadir}/entity-manager/configurations
    install -m 0444 ${UNPACKDIR}/blacklist.json ${D}${datadir}/entity-manager/
    install -m 0444 ${UNPACKDIR}/amd_cpu.json ${D}${datadir}/entity-manager/configurations/
    install -m 0444 ${UNPACKDIR}/dl110-g11.json ${D}${datadir}/entity-manager/configurations/
    install -m 0444 ${UNPACKDIR}/dl145-g11.json ${D}${datadir}/entity-manager/configurations/
    install -m 0444 ${UNPACKDIR}/dl320-g11.json ${D}${datadir}/entity-manager/configurations/
    install -m 0444 ${UNPACKDIR}/dl325-g11.json ${D}${datadir}/entity-manager/configurations/
    install -m 0444 ${UNPACKDIR}/dl345-g11.json ${D}${datadir}/entity-manager/configurations/
    install -m 0444 ${UNPACKDIR}/dl360-g11.json ${D}${datadir}/entity-manager/configurations/
    install -m 0444 ${UNPACKDIR}/dl365-g11.json ${D}${datadir}/entity-manager/configurations/
    install -m 0444 ${UNPACKDIR}/dl380-g11.json ${D}${datadir}/entity-manager/configurations/
    install -m 0444 ${UNPACKDIR}/dl380a-g11.json ${D}${datadir}/entity-manager/configurations/
    install -m 0444 ${UNPACKDIR}/dl385-g11.json ${D}${datadir}/entity-manager/configurations/
    install -m 0444 ${UNPACKDIR}/dl560-g11.json ${D}${datadir}/entity-manager/configurations/
    install -m 0444 ${UNPACKDIR}/hpe_drv.json ${D}${datadir}/entity-manager/configurations/
    install -m 0444 ${UNPACKDIR}/hpe_psu.json ${D}${datadir}/entity-manager/configurations/
    install -m 0444 ${UNPACKDIR}/rl300-g11.json ${D}${datadir}/entity-manager/configurations/
}
