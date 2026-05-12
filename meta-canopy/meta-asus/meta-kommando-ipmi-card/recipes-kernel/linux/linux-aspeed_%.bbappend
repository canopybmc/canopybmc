FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append:kommando-ipmi-card = " \
     file://aspeed-bmc-asus-kommando-ipmi-card.dts \
    "

do_configure:prepend:kommando-ipmi-card() {
    # copy the file to the kernem aspeed folder
    install -m 0644 ${UNPACKDIR}/aspeed-bmc-asus-kommando-ipmi-card.dts \
        ${S}/arch/arm/boot/dts/aspeed/aspeed-bmc-asus-kommando-ipmi-card.dts
}