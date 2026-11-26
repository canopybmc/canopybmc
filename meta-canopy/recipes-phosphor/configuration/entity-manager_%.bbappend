FILESEXTRAPATHS:prepend:canopy-qemu := "${THISDIR}/${PN}:"

SRC_URI:append:canopy-qemu = " file://canopy-qemu.json"

# fru-device reads every I2C address to find FRU EEPROMs. The emulated board
# has none, and a read of an emulated PMBus regulator crashes QEMU.
PACKAGECONFIG:remove:canopy-qemu = "ipmi-fru"

# The emulated board needs only its own config; the others cost flash space.
do_install:append:canopy-qemu() {
    rm -rf ${D}${datadir}/${BPN}/configurations
    install -D -m 0644 ${UNPACKDIR}/canopy-qemu.json \
        ${D}${datadir}/${BPN}/configurations/canopy/canopy-qemu.json
}
