FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# The vendored recipe lists fragments that vendor/ deletes
SRC_URI:remove = "file://ipmi_ssif.cfg file://mtd_test.cfg"

SRC_URI:append:ast2700-a1-dcscm = " \
    file://ast2700-a1-dcscm.dts \
    file://ast2700-a1-dcscm.cfg \
    "

do_configure:append:ast2700-a1-dcscm() {
    cp ${UNPACKDIR}/ast2700-a1-dcscm.dts ${S}/arch/arm64/boot/dts/aspeed/
}
