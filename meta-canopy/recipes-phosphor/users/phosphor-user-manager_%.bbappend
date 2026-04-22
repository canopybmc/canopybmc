FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
        file://0001-user-mgr-prevent-deletion-of-user-with-uid-0.patch \
        "
