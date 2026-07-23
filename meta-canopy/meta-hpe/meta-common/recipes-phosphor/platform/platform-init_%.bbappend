FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
        file://0001-build-allow-compiling-selected-platforms-only.patch \
        file://0002-Use-CLI11-callback-function-for-init-sub.patch \
        file://0003-Add-phosphor-logging-and-PDI-deps.patch \
        file://0004-cmos-reset-Add-cmos-reset-service.patch \
        file://0005-cmos-reset-Add-event-logs.patch \
        file://0006-cmos-reset-Add-power-control-capabilities.patch \
        file://0007-cmos-reset-Add-ActivationBlocksTransition.patch \
        "

EXTRA_OEMESON:append = " \
        -Dcmos-reset=enabled \
        "

DEPENDS:append = " \
        phosphor-dbus-interfaces \
        phosphor-logging \
        "

SYSTEMD_SERVICE:${PN}:append = " xyz.openbmc_project.Software.Host.Updater0.service "
