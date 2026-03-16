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

DEFAULTTUNE ?= "armv6-novfp"
require conf/machine/include/arm/arch-armv6.inc

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
