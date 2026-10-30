FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append:ast2700-a1-dcscm = " file://ast2700-a1-dcscm.dts"

do_configure:append:ast2700-a1-dcscm() {
    cp ${UNPACKDIR}/ast2700-a1-dcscm.dts ${S}/arch/arm64/boot/dts/aspeed/
}
