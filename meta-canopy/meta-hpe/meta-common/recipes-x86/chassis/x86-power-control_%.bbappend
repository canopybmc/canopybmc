FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
	file://power-config-host0.json \
	"

EXTRA_OEMESON:append = " \
    -Dbutton-passthrough=enabled \
    "

do_install:append() {
	install -d ${D}/${datadir}/${PN}
	install -m 0644 ${UNPACKDIR}/power-config-host0.json ${D}/${datadir}/${PN}
}
