FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
    file://kommando_ipmi_card.json \
"

PACKAGECONFIG:append = " dts-vpd"

do_install:append() {
    install -D ${UNPACKDIR}/kommando_ipmi_card.json ${D}${datadir}/${BPN}/configurations/asus/kommando_ipmi_card.json
}
