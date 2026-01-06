PREFERRED_PROVIDER_virtual/kernel ?= "linux-gxp"

PREFERRED_PROVIDER_virtual/bootloader ?= "u-boot-gxp"
PREFERRED_PROVIDER_u-boot ?= "u-boot-gxp"
PREFERRED_PROVIDER_u-boot-fw-utils ?= "u-boot-fw-utils-gxp"

PREFERRED_VERSION_u-boot-gxp ?= "v2020.10%"
PREFERRED_VERSION_u-boot-fw-utils-gxp ?= "v2020.10%"

PREFERRED_VERSION_linux-gxp ?= "5.14%"

# Build a signed FIT image by default.
KERNEL_IMAGETYPE = "fitImage"
KERNEL_CLASSES = "kernel-fitimage"
FITIMAGE_SIGN = "1"
FITIMAGE_HASH_ALGO = "sha512"
FIT_HASH_ALG = "sha512"
