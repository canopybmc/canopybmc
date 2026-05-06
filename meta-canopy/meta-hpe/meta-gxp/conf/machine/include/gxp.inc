#@TYPE: Machine
#@NAME: HPE GXP
#@DESCRIPTION: Common machine configuration for the HPE GXP Chip

require conf/machine/include/hpe_soc.inc
require conf/machine/include/gxp-flash-layout.inc

UBOOT_ARCH ?= "arm"

DISTRO_FEATURES ?= "ipv4 ipv6"

SOC_FAMILY = "gxp-b1"
include conf/machine/include/soc-family.inc
MACHINEOVERRIDES .= ":gxp"

require conf/machine/include/arm/armv7a/tune-cortexa9.inc

AVAILTUNES += "cortexa9t-novfp"
TUNE_FEATURES:tune-cortexa9t-novfp = "arm thumb cortexa9"
TUNE_CCARGS .= "${@bb.utils.contains('TUNE_FEATURES', 'cortexa9', ' -mcpu=cortex-a9', '', d)}"
ARMPKGARCH:tune-cortexa9t-novfp = "cortexa9"
PACKAGE_EXTRA_ARCHS:tune-cortexa9t-novfp = "${PACKAGE_EXTRA_ARCHS:tune-armv7at} cortexa9-vfp cortexa9t2 cortexa9t2-vfp"

DEFAULTTUNE = "cortexa9t-novfp"

UBOOT_MACHINE ?= "gxp_defconfig"
UBOOT_ENTRYPOINT ?= "0x40100000"
UBOOT_LOADADDRESS ?= "0x40100000"
UBOOT_DTB_LOADADDRESS ?= "0x48000000"
UBOOT_DTBO_LOADADDRESS ?= "0x48100000"

SERIAL_CONSOLES ?= "115200;ttyS2"

MACHINE_FEATURES += "\
    obmc-phosphor-chassis-mgmt \
    obmc-phosphor-fan-mgmt \
    obmc-phosphor-flash-mgmt \
    obmc-host-state-mgmt \
    obmc-chassis-state-mgmt \
    obmc-bmc-state-mgmt \
    obmc-host-ipmi \
    "

VIRTUAL-RUNTIME_obmc-host-state-manager ?= "x86-power-control"
VIRTUAL-RUNTIME_obmc-chassis-state-manager ?= "x86-power-control"
VIRTUAL-RUNTIME_obmc-sensors-hwmon = "dbus-sensors"
VIRTUAL-RUNTIME_obmc-fan-control = "phosphor-pid-control"

# HPE packagegroup providers
PREFERRED_PROVIDER_virtual/obmc-chassis-mgmt = "packagegroup-hpe-apps"
PREFERRED_PROVIDER_virtual/obmc-fan-mgmt = "packagegroup-hpe-apps"
PREFERRED_PROVIDER_virtual/obmc-flash-mgmt = "packagegroup-hpe-apps"
PREFERRED_PROVIDER_virtual/obmc-system-mgmt = "packagegroup-hpe-apps"
PREFERRED_PROVIDER_virtual/obmc-host-ipmi-hw = "phosphor-ipmi-kcs"
