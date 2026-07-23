FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
        file://0001-build-allow-compiling-selected-platforms-only.patch \
        "
