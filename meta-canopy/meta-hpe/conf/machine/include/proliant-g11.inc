GXP_BOARD_MAP = "\
    0x0235:dl360gen11 \
    0x0236:dl380gen11 \
    0x0241:dl560gen11 \
    0x0243:dl385gen11 \
    0x0244:dl325gen11 \
    0x0245:dl365gen11 \
    0x0246:dl345gen11 \
    0x0248:dl110gen11 \
    0x0249:dl320gen11 \
    0x024a:dl380agen11 \
    0x0250:rl300gen11 \
    0x0261:dl340gen11plus \
    0x0263:dl320gen11plus \
    0x0264:dl320gen12 \
    0x0271:dl145gen11 \
    0x0273:dl340gen12 \
"

KERNEL_DEVICETREE ?= "\
    hpe/hpe-bmc-dl360gen11.dtb \
    hpe/hpe-bmc-dl380gen11.dtb \
    hpe/hpe-bmc-dl560gen11.dtb \
    hpe/hpe-bmc-dl385gen11.dtb \
    hpe/hpe-bmc-dl325gen11.dtb \
    hpe/hpe-bmc-dl365gen11.dtb \
    hpe/hpe-bmc-dl345gen11.dtb \
    hpe/hpe-bmc-dl110gen11.dtb \
    hpe/hpe-bmc-dl320gen11.dtb \
    hpe/hpe-bmc-dl380agen11.dtb \
    hpe/hpe-bmc-rl300gen11.dtb \
    hpe/hpe-bmc-dl340gen11plus.dtb \
    hpe/hpe-bmc-dl320gen11plus.dtb \
    hpe/hpe-bmc-dl320gen12.dtb \
    hpe/hpe-bmc-dl145gen11.dtb \
    hpe/hpe-bmc-dl340gen12.dtb \
"

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

EXTRA_IMAGECMD:squashfs-xz ?= " -b 1M -Xbcj armthumb"

require conf/machine/include/gxp.inc
require conf/machine/include/obmc-bsp-common.inc
