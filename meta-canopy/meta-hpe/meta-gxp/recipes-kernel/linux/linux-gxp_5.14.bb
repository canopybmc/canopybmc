KBRANCH = "dev-5.14-gxp-openbmc"
LINUX_VERSION = "5.14"
SRCREV = "4ff23625306630ce50d9454fca590fa9b2daeb53"

require linux-gxp.inc

ERROR_QA:remove = "patch-status"

SRC_URI += " \
    file://0001-ipmi-kcs_bmc_gxp-add-missing-MODULE_LICENSE.patch \
    file://0002-hwmon-gxp-psu-add-missing-MODULE_LICENSE.patch \
    file://0003-hwmon-gxp-power-add-missing-MODULE_LICENSE.patch \
    "
