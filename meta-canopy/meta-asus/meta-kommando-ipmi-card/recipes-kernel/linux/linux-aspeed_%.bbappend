FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append:kommando-ipmi-card = " \
     file://pwm.cfg \
     file://pstore.cfg \
     file://aspeed-bmc-asus-kommando-ipmi-card.dts \
     file://v3-0001-dt-bindings-serial-8250-aspeed-add-compatible-str.patch \
     file://v3-0002-dt-bindings-serial-8250-aspeed-add-aspeed-vuart-o.patch \
     file://v3-0003-serial-8250_aspeed_vuart-add-aspeed-ast2600-vuart.patch \
     file://v3-0004-serial-8250_aspeed_vuart-add-VUART-over-PCI.patch \
     file://v3-0006-ARM-dts-aspeed-g6-Change-vuart-compatible-string-.patch \
     file://v3-0007-ARM-dts-aspeed-g6-add-aspeed-vuart-over-pci-prop-.patch \
    "

do_configure:prepend:kommando-ipmi-card() {
    # copy the new dts file to the dts aspeed folder
    install -m 0644 ${UNPACKDIR}/aspeed-bmc-asus-kommando-ipmi-card.dts \
        ${S}/arch/arm/boot/dts/aspeed/aspeed-bmc-asus-kommando-ipmi-card.dts
}
