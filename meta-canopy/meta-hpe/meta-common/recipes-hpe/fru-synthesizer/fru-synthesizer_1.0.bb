SUMMARY = "Synthesize IPMI FRU images from D-Bus inventory"
DESCRIPTION = "Builds IPMI FRU blobs from MachineContext (device-tree VPD) and \
smbios-mdr (host SMBIOS) inventory and writes them to /etc/fru, where fru-device \
publishes them via xyz.openbmc_project.FruDevice for the stock IPMI storage \
handlers. Used on platforms (e.g. HPE ProLiant Gen11) that have no physical \
baseboard/CPU/DIMM FRU EEPROMs."

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit meson pkgconfig systemd

DEPENDS = " \
    sdbusplus \
    phosphor-logging \
    boost \
    systemd \
    "

# fru-device must be present to consume the generated /etc/fru files.
RDEPENDS:${PN} += "fru-device"

SRC_URI = " \
    file://meson.build;subdir=${BP} \
    file://fru_encoder.hpp;subdir=${BP} \
    file://fru_encoder.cpp;subdir=${BP} \
    file://main.cpp;subdir=${BP} \
    file://fru-synthesizer.service;subdir=${BP} \
    "

S = "${UNPACKDIR}/${BP}"

SYSTEMD_SERVICE:${PN} = "fru-synthesizer.service"
