SUMMARY = "GXP CHIF Service"
DESCRIPTION = "CHIF (Channel Interface) service for HPE GXP BMC. \
Receives SMBIOS data from host BIOS via /dev/chif24 and triggers \
smbios-mdr for Redfish inventory population."

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

# Local source — will be replaced with a git:// URI once the repo is published
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
SRC_URI = "file://src;subdir=${BP} \
           file://test;subdir=${BP} \
           file://service_files;subdir=${BP} \
           file://meson.build;subdir=${BP} \
           file://meson.options;subdir=${BP} \
           "

S = "${UNPACKDIR}/${BP}"

inherit meson pkgconfig systemd

SYSTEMD_SERVICE:${PN} = "xyz.openbmc_project.GxpChif.service"

DEPENDS += " \
    systemd \
    sdbusplus \
    phosphor-dbus-interfaces \
    phosphor-logging \
    ${@bb.utils.contains('PTEST_ENABLED', '1', 'gtest', '', d)} \
    "

RDEPENDS:${PN} += "smbios-mdr"

FILES:${PN} += "${nonarch_base_libdir}/udev/rules.d"

EXTRA_OEMESON = " \
    -Dtests=${@bb.utils.contains('PTEST_ENABLED', '1', 'enabled', 'disabled', d)} \
"

do_install_ptest() {
        install -d ${D}${PTEST_PATH}/test
        cp -rf ${B}/test/*_test ${D}${PTEST_PATH}/test/
}
