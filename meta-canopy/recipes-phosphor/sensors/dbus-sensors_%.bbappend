FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-fan-catch-error-if-symlink-read-fails.patch \
    "
