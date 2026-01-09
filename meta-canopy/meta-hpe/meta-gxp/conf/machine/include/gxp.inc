#@TYPE: Machine
#@NAME: HPE GXP
#@DESCRIPTION: Common machine configuration for the HPE GXP Chip

require conf/machine/include/hpe_soc.inc

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

SERIAL_CONSOLES ?= "115200;ttyS0"

# HPE packagegroup provider
PREFERRED_PROVIDER_virtual/obmc-system-mgmt = "packagegroup-hpe-apps"
