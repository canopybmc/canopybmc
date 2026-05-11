FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append:kommando-ipmi-card = " \
     file://ast2600-asus-kommando-ipmi-card.dts \
    "

do_configure:prepend:kommando-ipmi-card() {
    # copy the file to the uboot repo
    install -m 0644 ${UNPACKDIR}/ast2600-asus-kommando-ipmi-card.dts \
        ${S}/arch/arm/dts/ast2600-asus-kommando-ipmi-card.dts

    # if the dtb doesnt allready exist; append the dtb output to the makefile
    if ! grep -q "ast2600-asus-kommando-ipmi-card.dtb" ${S}/arch/arm/dts/Makefile; then
        sed -i '/^dtb-$(CONFIG_ARCH_ASPEED)\s*+=/a\\tast2600-asus-kommando-ipmi-card.dtb \\' \
            ${S}/arch/arm/dts/Makefile
    fi
}