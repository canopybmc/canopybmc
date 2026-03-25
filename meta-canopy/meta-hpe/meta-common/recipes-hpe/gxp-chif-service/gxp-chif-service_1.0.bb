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

S = "${WORKDIR}/${BP}"

inherit meson pkgconfig systemd

SYSTEMD_SERVICE:${PN} = "xyz.openbmc_project.GxpChif.service"

DEPENDS += " \
    systemd \
    sdbusplus \
    phosphor-dbus-interfaces \
    phosphor-logging \
    "

RDEPENDS:${PN} += "smbios-mdr"

FILES:${PN} += "${nonarch_base_libdir}/udev/rules.d"

# Pre-create the persistent EV storage directory so it exists in the
# rootfs image before the overlay is mounted. Without this, the CHIF
# service fails to create it at early boot.
do_install:append() {
    install -d ${D}${localstatedir}/lib/chif
}

EXTRA_OEMESON = "-Dtests=disabled"
