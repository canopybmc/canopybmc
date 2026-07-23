FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
    file://kommando_ipmi_card.json \
"

do_install:append() {
    # Remove all default configs 
    # The card doesn't have any direct access to FRU
    # This saves us ~3.2 MiB in rofs.
    find ${D}${datadir}/${BPN}/configurations \
        -mindepth 1 -maxdepth 1 \
        -type d \
        -exec rm -rf {} +

    install -D ${UNPACKDIR}/kommando_ipmi_card.json ${D}${datadir}/${BPN}/configurations/asus/kommando_ipmi_card.json
}
