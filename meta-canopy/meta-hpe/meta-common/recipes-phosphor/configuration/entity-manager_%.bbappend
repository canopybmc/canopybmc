FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0002-schemas-firmware-allow-for-updating-spi-partition.patch \
    file://0003-fru-device-load-synthetic-FRUs-from-etc-fru.patch \
    file://0004-utils-allow-specifying-a-devicetree-path-as-i2c-bus.patch \
"

SRC_URI:append = " \
    file://blacklist.json \
    file://dl110g11_baseboard.json \
    file://dl145g11_baseboard.json \
    file://dl320g11_baseboard.json \
    file://dl325g11_baseboard.json \
    file://dl345g11_baseboard.json \
    file://dl360g11_baseboard.json \
    file://dl365g11_baseboard.json \
    file://dl380ag11_baseboard.json \
    file://dl380g11_baseboard.json \
    file://dl385g11_baseboard.json \
    file://dl560g11_baseboard.json \
    file://rl300g11_baseboard.json \
    file://hpe_psu.json \
    file://hpe_drv.json \
    file://hpe_ubm.json \
"

PACKAGECONFIG:append = " dts-vpd"

do_install:append() {
    install -D ${UNPACKDIR}/blacklist.json ${D}${datadir}/${BPN}/blacklist.json
    # Remove all default configs except for HPE ones.
    # This saves us valuable space in rofs.
    find ${D}${datadir}/${BPN}/configurations \
        -mindepth 1 -maxdepth 1 \
        -type d \
        -not -name "hpe" \
        -exec rm -rf {} +

    install -D ${UNPACKDIR}/dl110g11_baseboard.json ${D}${datadir}/${BPN}/configurations/hpe/dl110g11_baseboard.json
    install -D ${UNPACKDIR}/dl145g11_baseboard.json ${D}${datadir}/${BPN}/configurations/hpe/dl145g11_baseboard.json
    install -D ${UNPACKDIR}/dl320g11_baseboard.json ${D}${datadir}/${BPN}/configurations/hpe/dl320g11_baseboard.json
    install -D ${UNPACKDIR}/dl325g11_baseboard.json ${D}${datadir}/${BPN}/configurations/hpe/dl325g11_baseboard.json
    install -D ${UNPACKDIR}/dl345g11_baseboard.json ${D}${datadir}/${BPN}/configurations/hpe/dl345g11_baseboard.json
    install -D ${UNPACKDIR}/dl360g11_baseboard.json ${D}${datadir}/${BPN}/configurations/hpe/dl360g11_baseboard.json
    install -D ${UNPACKDIR}/dl365g11_baseboard.json ${D}${datadir}/${BPN}/configurations/hpe/dl365g11_baseboard.json
    install -D ${UNPACKDIR}/dl380ag11_baseboard.json ${D}${datadir}/${BPN}/configurations/hpe/dl380ag11_baseboard.json
    install -D ${UNPACKDIR}/dl380g11_baseboard.json ${D}${datadir}/${BPN}/configurations/hpe/dl380g11_baseboard.json
    install -D ${UNPACKDIR}/dl385g11_baseboard.json ${D}${datadir}/${BPN}/configurations/hpe/dl385g11_baseboard.json
    install -D ${UNPACKDIR}/dl560g11_baseboard.json ${D}${datadir}/${BPN}/configurations/hpe/dl560g11_baseboard.json
    install -D ${UNPACKDIR}/rl300g11_baseboard.json ${D}${datadir}/${BPN}/configurations/hpe/rl300g11_baseboard.json
    install -D ${UNPACKDIR}/hpe_psu.json ${D}${datadir}/${BPN}/configurations/hpe/hpe_psu.json
    install -D ${UNPACKDIR}/hpe_drv.json ${D}${datadir}/${BPN}/configurations/hpe/hpe_drv.json
    install -D ${UNPACKDIR}/hpe_ubm.json ${D}${datadir}/${BPN}/configurations/hpe/hpe_ubm.json
}
