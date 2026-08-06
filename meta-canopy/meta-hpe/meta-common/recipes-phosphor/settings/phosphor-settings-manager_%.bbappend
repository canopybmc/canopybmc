FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
SRC_URI:append = " file://power_settings.override.yml"
SRC_URI:append = " file://chassis_capabilities.override.yml"
SRC_URI:append = " file://power_on_hours.override.yml"
