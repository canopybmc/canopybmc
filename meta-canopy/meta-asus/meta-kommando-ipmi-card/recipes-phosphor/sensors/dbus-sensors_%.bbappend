FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-hwmontempsensor-add-support-for-internal-NTC.patch \
    "
