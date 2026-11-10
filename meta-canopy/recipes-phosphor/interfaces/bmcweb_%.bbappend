FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-network_protocol-support-OpenSSH-as-SSH-server.patch \
    "

PACKAGECONFIG:append:df-dbus-logging = " redfish-dbus-log"

PACKAGECONFIG:append = " \
        redfish-dump-log \
"
