PREFERRED_PROVIDER_virtual/kernel ?= "linux-stable"

# FIT images are now built by the linux-yocto-fitimage recipe
# The kernel itself builds a standard image type (zImage for ARM)
KERNEL_CLASSES ?= "kernel-fit-extra-artifacts"
KERNEL_IMAGETYPE ?= "zImage"
KERNEL_IMAGETYPES ?= "zImage"
FIT_KERNEL_SIGN_ENABLE = "1"
FIT_HASH_ALG = "sha512"
FIT_SIGN_ALG = "rsa4096"
UBOOT_SIGN_ENABLE = "1"
UBOOT_SIGN_KEYNAME = "hpe_fit"
