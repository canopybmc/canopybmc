KERNEL_DEVICETREE ?= "hpe/hpe-gxp.dtb"
KERNEL_DTBVENDORED ?= "1"

# bootblock loader variants in use
HPE_GXP_LOADERS = "\
  GXP2loader-t277-t280-t285-sgn00 \
  GXP2loader-t282-t288-sgn00 \
  GXP2loader-t26x-sgn00 \
"

# Map each loader to gen11 model names
HPE_GXP_LOADER_MODELS[GXP2loader-t277-t280-t285-sgn00] = "\
  dl320-g11 \
  dl325-g11 \
  dl360-g11 \
  dl365-g11 \
  dl380-g11 \
  dl385-g11 \
"
HPE_GXP_LOADER_MODELS[GXP2loader-t26x-sgn00] = "\
  rl300-g11 \
"

require conf/machine/include/gxp.inc
require conf/machine/include/obmc-bsp-common.inc
