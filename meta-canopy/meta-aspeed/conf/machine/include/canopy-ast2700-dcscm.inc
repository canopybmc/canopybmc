MACHINEOVERRIDES =. "canopy-ast2700-dcscm:"

require conf/machine/include/ast-ssp.inc
require conf/machine/include/ast-tsp.inc
require conf/machine/include/ast2700-secure-customize-gen.inc
require conf/machine/include/obmc-bsp-common.inc

ASPEED_CUSTOMIZE_GEN_SECURE_IMAGE_ENABLE = "0"

# 64 MiB BMC flash layout.
FLASH_SIZE = "65536"
FLASH_UBOOT_OFFSET:flash-65536 = "0"
FLASH_UBOOT_ENV_OFFSET:flash-65536 = "4096"
FLASH_KERNEL_OFFSET:flash-65536 = "4224"
FLASH_ROFS_OFFSET:flash-65536 = "13440"
FLASH_RWFS_OFFSET:flash-65536 = "50944"

UBOOT_MACHINE = "evb-ast2700_defconfig"
UBOOT_DEVICETREE = "ast2700-dcscm"

KBUILD_DEFCONFIG = "aspeed_g7_defconfig"

MACHINE_FEATURES += "\
        obmc-phosphor-flash-mgmt \
        obmc-bmc-state-mgmt \
        "

OBMC_HOST_INSTANCES = ""
OBMC_CHASSIS_INSTANCES = ""
VIRTUAL-RUNTIME_obmc-fan-presence = ""

EXTRA_IMAGEDEPENDS:append = " \
    aspeed-image-recoveryuart \
    "
