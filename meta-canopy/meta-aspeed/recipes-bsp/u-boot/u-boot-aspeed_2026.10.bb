# Copy of the SPI NOR parts of the recipe in openbmc change 81057, which
# this template cannot pick up because it leaves out openbmc/meta-aspeed.
# https://gerrit.openbmc.org/c/openbmc/openbmc/+/81057
require recipes-bsp/u-boot/u-boot-common.inc
require recipes-bsp/u-boot/u-boot.inc

DEPENDS += "bc-native dtc-native gnutls-native python3-pyelftools-native"

PROVIDES += "u-boot"

# Install the default environment file expected by libubootenv.
UBOOT_INITIAL_ENV = "u-boot-initial-env"

UBRANCH = "v2026.10-openbmc"
SRC_URI = "git://github.com/openbmc/u-boot.git;protocol=https;branch=${UBRANCH}"
SRCREV = "196cee41b40231fed472b52373ce1213d74287f3"

SRC_URI += " \
    file://0001-ram-aspeed-ast2700-read-the-DRAM-size-when-init-is-s.patch \
    file://0002-clk-ast2700-bind-the-generic-syscon-to-the-SCU-node.patch \
    file://0003-arm-dts-ast2700-evb-mux-the-FMC-quad-SPI-pins.patch \
    file://0004-net-ftgmac100-Add-RGMII-delay-support-for-AST2600.patch \
    file://0005-net-ftgmac100-Add-RGMII-delay-support-for-AST2700.patch \
    file://0006-arm-dts-ast2600-add-aspeed-scu-property-for-MAC.patch \
    file://0007-arm-dts-ast2600-evb-Configure-RGMII-delay-for-MAC.patch \
    file://0008-arm-dts-ast2700-add-aspeed-scu-property-for-MAC.patch \
    file://0009-arm-dts-ast2700-evb-Set-TI-DP83867-internal-delay-fo.patch \
    file://0010-arm-dts-ast2700-evb-mux-the-MDIO-and-RGMII-pins.patch \
    file://0011-arm-dts-aspeed-add-the-AST2700-DC-SCM-card.patch \
"

# The SDK U-Boot and kernel enable the Airoha AN8801 PHY driver too.
SRC_URI:append:ast2700-a1-dcscm = " file://ast2700-a1-dcscm.cfg"
