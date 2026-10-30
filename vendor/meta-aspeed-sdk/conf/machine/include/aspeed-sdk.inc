PREFERRED_PROVIDER_virtual/kernel ?= "linux-aspeed"
PREFERRED_PROVIDER_virtual/bootloader ?= "u-boot-aspeed-sdk"
PREFERRED_PROVIDER_u-boot ?= "u-boot-aspeed-sdk"
PREFERRED_PROVIDER_u-boot-fw-utils ?= "u-boot-fw-utils-aspeed-sdk"

# FIT images are now built by the linux-yocto-fitimage recipe
# The kernel itself builds a standard image type (zImage for ARM, Image for ARM64)
KERNEL_IMAGETYPE ?= "zImage"
KERNEL_IMAGETYPE:aarch64 ?= "Image"
KERNEL_IMAGETYPES:aarch64 ?= "Image"
KERNEL_CLASSES ?= "kernel-fit-extra-artifacts"
INITRAMFS_IMAGE ?= "obmc-phosphor-initramfs"
INITRAMFS_FSTYPES ?= "cpio.xz"

WKS_FILE ?= "ast2600-emmc.wks.in"
WKS_FILE_DEPENDS ?= " \
    phosphor-hostfw-image \
    virtual/bootloader \
    virtual/kernel \
    linux-yocto-fitimage \
"
WKS_HOSTFW_SIZE ?= "5G"
IMAGE_BOOT_FILES ?= "fitImage-${INITRAMFS_IMAGE}-${MACHINE}-${MACHINE};fitImage"

MACHINE_EXTRA_RRECOMMENDS += "udev-aspeed-vuart udev-aspeed-mtd-partitions"
MACHINE_FEATURES += "hw-rng"

MACHINEOVERRIDES =. "aspeed:"

PREFERRED_VERSION_iperf3 ?= "3.1.4"

# Required to explicitly define the oldest kernel version supported by the ASPEED SDK.
require oldest-kernel.inc

# Installs static development packages, which are static libraries (i.e. *.a files), for all packages installed in a given SDK
SDKIMAGE_FEATURES += "staticdev-pkgs"

# Set VIRTUAL-RUNTIME for obmc-fan-control and obmc-sensors-hwmon.
VIRTUAL-RUNTIME_obmc-fan-control = "phosphor-pid-control"
VIRTUAL-RUNTIME_obmc-fan-presence = "phosphor-pid-control"
VIRTUAL-RUNTIME_obmc-sensors-hwmon = "dbus-sensors"

# OpenBMC phosphor-defaults.inc sets this to phosphor-ipmi-fru.
# Remove it to use the phosphor-host-ipmid dbus-sdr FRU write command.
VIRTUAL-RUNTIME_phosphor-ipmi-providers = ""
