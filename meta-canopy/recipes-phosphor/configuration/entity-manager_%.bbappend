FILESEXTRAPATHS:prepend:canopy-qemu := "${THISDIR}/${PN}:"

SRC_URI:append:canopy-qemu = " file://canopy-qemu.json"

# The emulated board needs only its own config; the others cost flash space.
do_install:append:canopy-qemu() {
    rm -rf ${D}${datadir}/${BPN}/configurations
    install -D -m 0644 ${UNPACKDIR}/canopy-qemu.json \
        ${D}${datadir}/${BPN}/configurations/canopy/canopy-qemu.json
}
