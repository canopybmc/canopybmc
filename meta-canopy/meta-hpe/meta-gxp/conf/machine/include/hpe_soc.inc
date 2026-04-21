PREFERRED_PROVIDER_virtual/kernel ?= "linux-stable"

# FIT images are now built by the linux-yocto-fitimage recipe
# The kernel itself builds a standard image type (zImage for ARM)
KERNEL_CLASSES ?= "kernel-fit-extra-artifacts"
KERNEL_IMAGETYPE ?= "zImage"
KERNEL_IMAGETYPES ?= "zImage"
FITIMAGE_SIGN = "1"
FITIMAGE_HASH_ALGO = "sha512"
FIT_HASH_ALG = "sha512"
