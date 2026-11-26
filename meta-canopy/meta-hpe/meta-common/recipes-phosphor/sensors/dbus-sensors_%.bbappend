FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# The generic meta-canopy patch is appended after the patches below, but
# 0008-0016 are based on it. 0007 is a copy of it applied before them.
SRC_URI:remove = "file://0001-fan-catch-error-if-symlink-read-fails.patch"

SRC_URI:append = " \
    file://0001-reduce-image-size-by-moving-to-Multi-call-Binary.patch \
    file://0004-hwmontempsensor-add-platform-device-support.patch \
    file://0005-psusensor-add-gxp_psu-to-sensorTypes-whitelist.patch \
    file://0006-HwmonTempSensor-Add-support-for-SPD5118.patch \
    file://0007-fan-catch-error-if-symlink-read-fails.patch \
    file://0008-fansensor-do-not-exit-when-a-presence-GPIO-cannot-be.patch \
    file://0009-fansensor-move-configuration-handling-out-of-the-lam.patch \
    file://0010-fansensor-extract-fan-configuration-lookup.patch \
    file://0011-fansensor-extract-presence-GPIO-setup.patch \
    file://0012-fansensor-extract-connector-setup.patch \
    file://0013-fansensor-move-fan-inventory-item-out-of-TachSensor.patch \
    file://0014-fansensor-support-fans-without-a-tachometer.patch \
    file://0015-fansensor-publish-fans-as-Inventory.Item.Fan.patch \
    file://0016-fansensor-report-fanN_fault-as-OperationalStatus.patch \
    "

ERROR_QA:remove = "patch-status"

# Disable sensor daemons that have no matching
# entity-manager configuration on HPE platforms and run idle.
PACKAGECONFIG:remove = "exitairtempsensor external ipmbsensor mcutempsensor"

# Enable the NVMe sensor daemon
PACKAGECONFIG:append = " nvmesensor"
