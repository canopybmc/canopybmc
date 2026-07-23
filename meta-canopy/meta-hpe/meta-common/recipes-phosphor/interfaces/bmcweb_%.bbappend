FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
        file://0001-OpenBmc-org-BiosReset.patch \
        "

PACKAGECONFIG:append = " redfish-allow-deprecated-power-thermal"

EXTRA_OEMESON:append = " \
        -Dhttp-body-limit=64 \
        "
