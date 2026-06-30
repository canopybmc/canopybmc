FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-devicetree-vpd-add-part-number-parsing.patch"
SRC_URI += "file://0002-devicetree-vpd-add-manufacturer-parsing.patch"
SRC_URI += "file://0003-entity-manager-move-D-Bus-name-and-path-to-shared-co.patch"
SRC_URI += "file://0004-entity-manager-avoid-probing-own-D-Bus-name.patch"
ERROR_QA:remove = "patch-status"

# Enable devicetree VPD parser for platform identification
PACKAGECONFIG:append = " dts-vpd"

# Baseboard (PCA) VPD support for devicetree-vpd-parser
SRC_URI += "file://0001-devicetree-vpd-parser-add-baseboard-PCA-VPD-support.patch"

# HPE ProLiant Gen11 baseboard configurations
SRC_URI += " \
    file://blocklist.json \
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
"

do_install:append() {
    install -D ${UNPACKDIR}/blocklist.json ${D}${datadir}/${BPN}/blacklist.json
    # Remove all default configs except for some vendors like NIC,
    # OCP and NVMes.
    # This saves us ~3 MiB in rofs.
    find ${D}${datadir}/${BPN}/configurations \
        -mindepth 1 -maxdepth 1 \
        -type d \
        -not -name "broadcomm" \
        -not -name "hpe" \
        -not -name "intel" \
        -not -name "micron" \
        -not -name "ocp" \
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
}
