FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
        file://0001-account-service-prevent-deletion-of-own-account.patch \
        file://0002-account-service-return-proper-error-on-deletion.patch \
        file://0003-http-zstd-do-not-attempt-compressing-already-opened-.patch \
"

PACKAGECONFIG:append = " \
        redfish-dbus-log \
        redfish-dump-log \
"
