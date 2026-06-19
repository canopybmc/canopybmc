FILESEXTRAPATHS:prepend := "${THISDIR}/${BPN}:"

PACKAGECONFIG:append = " dynamic-sensors"

SRC_URI:append = " file://0001-apphandler-fall-back-to-ObjectMapper-when-settings-U.patch"
