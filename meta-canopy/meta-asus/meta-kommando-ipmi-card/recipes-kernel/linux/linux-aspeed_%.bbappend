FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append:kommando-ipmi-card = " \
     file://kommando-ipmi-card.cfg \
     file://aspeed-bmc-asus-kommando-ipmi-card.dts \
     file://pcie-lpc/v2-0001-ipmi-kcs_bmc_aspeed-g6-Add-KCS-Channel-4-over-PCI.patch \
     file://pcie-lpc/v2-0002-ARM-dts-aspeed-g6-add-pcie-kcs4.patch \
    "

do_configure:prepend:kommando-ipmi-card() {
    # copy the file to the kernem aspeed folder
    install -m 0644 ${UNPACKDIR}/aspeed-bmc-asus-kommando-ipmi-card.dts \
        ${S}/arch/arm/boot/dts/aspeed/aspeed-bmc-asus-kommando-ipmi-card.dts
}
