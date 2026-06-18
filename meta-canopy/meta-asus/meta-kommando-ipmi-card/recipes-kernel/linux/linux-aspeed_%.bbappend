FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append:kommando-ipmi-card = " \
     file://pwm.cfg \
     file://bmc-dev.cfg \
     file://pstore.cfg \
     file://aspeed-bmc-asus-kommando-ipmi-card.dts \
     file://v2-0001-soc-aspeed-add-BMC-side-PCIe-BMC-device-driver.patch \
    "

do_configure:prepend:kommando-ipmi-card() {
    # copy the new dts file to the dts aspeed folder
    install -m 0644 ${UNPACKDIR}/aspeed-bmc-asus-kommando-ipmi-card.dts \
        ${S}/arch/arm/boot/dts/aspeed/aspeed-bmc-asus-kommando-ipmi-card.dts
}
