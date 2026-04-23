FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
        file://0001-account-service-prevent-deletion-of-own-account.patch \
"

PACKAGECONFIG:append = " \
        redfish-dbus-log \
        redfish-dump-log \
"
