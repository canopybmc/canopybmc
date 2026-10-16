FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
        file://0001-account-service-prevent-deletion-of-own-account.patch \
        file://0002-http-abort-request-correctly-if-body-is-bigger-than-.patch \
"

PACKAGECONFIG:append:df-dbus-logging = " redfish-dbus-log"

PACKAGECONFIG:append = " \
        redfish-dump-log \
"
