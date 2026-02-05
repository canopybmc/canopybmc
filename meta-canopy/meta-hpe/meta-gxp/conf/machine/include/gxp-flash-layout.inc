# GXP BMC Flash Partition Layout

# GXP SoC fixed constants
FLASH_SIZE = "32256"
FLASH_UBOOT_OFFSET = "0"
FLASH_KERNEL_OFFSET = "512"

FLASH_UBOOT_SIZE = "384"
FLASH_UBOOT_ENV_OFFSET = "448"
FLASH_UBOOT_ENV_SIZE = "128"
FLASH_UBOOT_SIG_SIZE = "64"
FLASH_GXP_BOOTBLOCK_SIZE = "256"
FLASH_GXP_CKB_SIZE = "256"

# Full ROM size (BMC + bootblock + CKB = 32 MiB)
FLASH_ROM_SIZE = "32768"

# Offset where the uboot-sig partition starts (= FLASH_SIZE - SIG_SIZE)
FLASH_UBOOT_SIG_OFFSET = "32192"

# Offset where the GXP hardware section starts (= FLASH_SIZE)
FLASH_GXP_OFFSET = "32256"

# Tunable partition boundaries
FLASH_ROFS_OFFSET ?= "5376"
FLASH_RWFS_OFFSET ?= "32000"
