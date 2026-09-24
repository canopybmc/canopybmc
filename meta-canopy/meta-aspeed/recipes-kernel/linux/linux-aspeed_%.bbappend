FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append:ast2700dcscm = " file://ast2700dcscm.dts"

do_configure:append:ast2700dcscm() {
    cp ${UNPACKDIR}/ast2700dcscm.dts ${S}/arch/arm64/boot/dts/aspeed/
}

SRC_URI:append:ast2700a1dcscm = " file://ast2700a1dcscm.dts"

do_configure:append:ast2700a1dcscm() {
    cp ${UNPACKDIR}/ast2700a1dcscm.dts ${S}/arch/arm64/boot/dts/aspeed/
}
