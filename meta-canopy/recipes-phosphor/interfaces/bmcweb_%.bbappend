FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-network_protocol-support-OpenSSH-as-SSH-server.patch \
    file://0002-persistent_data-restore-the-session-timeout.patch \
    "

PACKAGECONFIG:append:df-dbus-logging = " redfish-dbus-log"

PACKAGECONFIG:append = " \
        redfish-dump-log \
"
