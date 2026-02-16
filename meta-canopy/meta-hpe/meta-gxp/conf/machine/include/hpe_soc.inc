PREFERRED_PROVIDER_virtual/kernel ?= "linux-stable"

# Build a signed FIT image by default.
KERNEL_IMAGETYPE = "fitImage"
KERNEL_CLASSES = "kernel-fitimage"
FITIMAGE_SIGN = "1"
FITIMAGE_HASH_ALGO = "sha512"
FIT_HASH_ALG = "sha512"
