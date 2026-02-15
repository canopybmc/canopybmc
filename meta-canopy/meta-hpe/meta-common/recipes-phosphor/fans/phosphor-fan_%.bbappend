FILESEXTRAPATHS:append := "${THISDIR}/${PN}:"

PACKAGECONFIG:append = " json"

SRC_URI:append = " file://presence.json"

do_configure:prepend() {
        mkdir -p ${S}/presence/config_files/${MACHINE}
        cp ${UNPACKDIR}/presence.json ${S}/presence/config_files/${MACHINE}/config.json
}
