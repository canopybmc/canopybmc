FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
    file://blocklist.json \
    file://kommando_ipmi_card.json \
"

do_install:append() {
    install -D ${UNPACKDIR}/blocklist.json ${D}${datadir}/${BPN}/blacklist.json
    # Remove all default configs except for some vendors like NIC,
    # OCP and NVMes.
    # This saves us ~3 MiB in rofs.
    find ${D}${datadir}/${BPN}/configurations \
        -mindepth 1 -maxdepth 1 \
        -type d \
        -not -name "broadcomm" \
        -not -name "hpe" \
        -not -name "intel" \
        -not -name "micron" \
        -not -name "ocp" \
        -exec rm -rf {} +

    install -D ${UNPACKDIR}/kommando_ipmi_card.json ${D}${datadir}/${BPN}/configurations/asus/kommando_ipmi_card.json
}
