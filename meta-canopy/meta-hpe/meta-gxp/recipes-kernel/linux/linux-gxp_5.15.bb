KBRANCH = "dev-5.15-gxp-openbmc"
LINUX_VERSION = "5.15"
SRCREV = "b77336354649420d5b97bc3cf46fb5f8ed9c2861"

require linux-gxp.inc

ERROR_QA:remove = "patch-status"

SRC_URI += " \
    file://0001-ipmi-kcs_bmc_gxp-add-missing-MODULE_LICENSE.patch \
    file://0002-hwmon-gxp-psu-add-missing-MODULE_LICENSE.patch \
    file://0003-hwmon-gxp-power-add-missing-MODULE_LICENSE.patch \
    "
