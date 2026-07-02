FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
SRC_URI:append = " \
    file://0001-reduce-image-size-by-moving-to-Multi-call-Binary.patch \
    file://0002-fansensor-support-TachInput-configuration-for-PWM-on.patch \
    file://0003-fansensor-add-fan-fault-monitoring-via-hwmon-fault-a.patch \
    file://0004-hwmontempsensor-add-platform-device-support.patch \
    file://0005-psusensor-add-gxp_psu-to-sensorTypes-whitelist.patch \
    file://0006-HwmonTempSensor-Add-support-for-SPD5118.patch \
    "

ERROR_QA:remove = "patch-status"

# Disable sensor daemons that have no matching
# entity-manager configuration on HPE platforms and run idle.
PACKAGECONFIG:remove = "exitairtempsensor external ipmbsensor mcutempsensor"

# Enable the NVMe sensor daemon
PACKAGECONFIG:append = " nvmesensor"
