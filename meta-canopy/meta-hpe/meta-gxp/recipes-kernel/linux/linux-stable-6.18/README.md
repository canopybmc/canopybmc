# Patch Overview

Every patch should contain an "Upstream-Status" tag as described in the
[Yocto doc](https://docs.yoctoproject.org/current/contributor-guide/recipe-style-guide.html#patch-upstream-status).

## Pure fixes to upstream

- 0001 # spi: gxp: fix out-of-bounds access in memory-mapped read
- 0002 # spi: gxp: support addressed reads with dummy cycles
- 0003 # i2c: gxp: fix use of error pointer on syscon lookup failure
- 0090 # i2c: gxp: support SMBus block read (I2C_M_RECV_LEN)
- 0004 # mtd: spi-nor: macronix: allow MX66L51235F probe without SFDP

## Changes to upstream

- 0005 # dt-bindings: arm: hpe: simplify to generic GXP compatible
- 0006 # ARM: dts: hpe-gxp: convert to standalone device tree

## Additions

- 0007 # ARM: dts: hpe-gxp: expand device tree for boot support
- 0089 # ARM: dts: hpe: gxp: add nvram partitions

### virtual EEPROM

- 0008 # dt-bindings: nvmem: add HPE GXP virtual EEPROM binding
- 0009 # nvmem: add HPE GXP virtual EEPROM driver
- 0010 # ARM: dts: hpe-gxp: add NVMEM virtual EEPROM with MAC address cells

### UMAC ethernet controller

- 0011 # dt-bindings: net: add HPE GXP UMAC ethernet controller bindings
- 0012 # net: ethernet: add HPE GXP UMAC driver
- 0013 # ARM: dts: hpe-gxp: add UMAC ethernet and MDIO nodes

### GXP SOC

- 0014 # dt-bindings: soc: hpe: add GXP SoC subsystem bindings
- 0015 # soc: hpe: add GXP SoC infrastructure drivers
- 0016 # gpio: add HPE GXP GPIO controller driver

### power, temp, cooling

#### GXP fan control (change/addition)
- 0017 # dt-bindings: hwmon: hpe,gxp-fan-ctrl: use syscon phandles
- 0018 # hwmon: gxp-fan-ctrl: use syscon phandles for XREG and FN2

#### GXP host power controller (new/addition)

- 0019 # dt-bindings: soc: hpe: add GXP host power controller binding
- 0020 # soc: hpe: add GXP host power controller driver
- 0021 # ARM: dts: hpe-gxp: add SoC infrastructure and power control
    - TODO: ? only regarding power controller. so split out LED,…

### Virtual UART

- 0027 # dt-bindings: serial: add HPE GXP Virtual UART binding
- 0028 # serial: 8250: add HPE GXP Virtual UART driver
- 0029 # ARM: dts: hpe-gxp: add Virtual UART node

### CHIF

- 0030 # dt-bindings: soc: hpe: add GXP CHIF binding
- 0031 # soc: hpe: add GXP CHIF driver
    - 0088 # soc: hpe: gxp-chif: expose poll on char device
- 0032 # ARM: dts: hpe-gxp: add CHIF node

### CPLD host power supply

- 0033 # dt-bindings: regulator: add HPE GXP CPLD host power supply
- 0034 # regulator: gxp-cpld: add HPE GXP CPLD host power supply driver
- 0035 # dt-bindings: regulator: hpe,gxp-host-power-supply: add interrupts

#### HWMON fan control

- 0037 # dt-bindings: hwmon: gxp-fan-ctrl: replace fn2 syscon with fan-supply
- 0038 # hwmon: gxp-fan-ctrl: use fan-supply regulator and fix sysfs semantics
    - 0043 # hwmon: gxp-fan-ctrl: expose fan_input reporting PWM duty cycle
- 0039 # dt-bindings: soc: hpe: gxp-fn2: allow regulator child nodes

#### needs CLPD & fan-supply (not directly related)

- 0040 # ARM: dts: hpe-gxp: add CPLD host power regulator and fan-supply
- 0041 # ARM: dts: hpe-gxp: add PGOOD interrupt to host power regulator
- 0042 # regulator: gxp-cpld: debounce PGOOD IRQ before notifying consumers

- 0044 # ARM: dts: hpe-gxp: add gpio-keys-polled node for fan presence

### SOC temp

- 0046 # dt-bindings: hwmon: add HPE GXP SoC temperature sensor binding
- 0047 # hwmon: gxp-coretemp: add HPE GXP SoC temperature sensor driver
- 0048 # ARM: dts: hpe-gxp: add coretemp sensor node

### PECI

- 0049 # peci: core: export device lifecycle helpers for controller drivers
- 0050 # peci: request: retry on unrecognized completion codes
- 0051 # dt-bindings: peci: add HPE GXP PECI controller binding
- 0052 # peci: controller: add HPE GXP PECI controller driver
- 0053 # ARM: dts: hpe-gxp: add PECI controller node

- 0056 # peci: core: serialize device creation with per-controller mutex

#### Upstreamed (Mainline/OpenBMC) but not yet in Canopy

Should go under the `backports` directory.

- 0057 # peci: cpu: add Intel Emerald Rapids support
    - [Mainline](https://github.com/torvalds/linux/commit/906f25050add51f1a412ea37e618d8748f75e23a)
    - [OpenBMC](https://github.com/openbmc/linux/commit/3e8577cf510b57d7eed49410892c9e3fb063a8b6)
- 0058 # hwmon: (peci/cputemp) add Intel Emerald Rapids support
    - [Mainline](https://github.com/torvalds/linux/commit/a45b3ae40451542e3d6b37b8fba04e280cc8efa7)
    - [OpenBMC](https://github.com/openbmc/linux/commit/ba57dbcf37aa98d790e14f718ebd53e2850cd979)
- 0059 # hwmon: (peci/dimmtemp) add Intel Emerald Rapids platform support
    - [Mainline](https://github.com/torvalds/linux/commit/03c5ecc276fdc696ec469ee3a784726b809ecf26)
    - [OpenBMC](https://github.com/openbmc/linux/commit/0e1fd5bd81115c687759cb01f2d25a0197b33500)

### Fan speed on BMC reboot/shutdown/kernel panic

- 0060 # dt-bindings: hwmon: gxp-fan-ctrl: add fan-shutdown-percent property
- 0061 # hwmon: gxp-fan-ctrl: restore safe fan speed on shutdown and removal
- 0062 # ARM: dts: hpe-gxp: set fan shutdown speed to 50 percent
- 0063 # hwmon: gxp-fan-ctrl: restore fan speed on kernel panic

### unrelated? undecided

- 0065 # ARM: dts: hpe-gxp: add mmio-mux for i2c bus mux select registers

### GXP PSU

- 0066 # hwmon: add HPE GXP PSU driver
    - TODO: add Signed-off-by by original (HPE) authors

### regulators?

- 0068 # hwmon: sbtsi_temp: add regulator supply and probe deferral support
- 0069 # misc: sbrmi: add regulator supply support
- 0070 # misc: amd-sbi: add SB-RMI revision 0x21 (Turin) protocol support

### I2C passthrough

- 0071 # dt-bindings: soc: hpe: add GXP I2C passthrough binding
- 0072 # soc: hpe: add GXP I2C passthrough driver
- 0073 # ARM: dts: hpe-gxp: add I2C passthrough node

### KCS

- 0074 # dt-bindings: soc: hpe: add GXP KCS Interface binding
    - TODO: proper commit description
- 0075 # soc: hpe: add GXP KCS interface driver
    - TODO: proper commit description
    - TODO: original (HPE) authors: Co-developed-by tag?

### Video capture engine

- 0076 # dt-bindings: media: add HPE GXP Thumbnail Video Capture
- 0077 # media: add GXP thumbnail video capture driver
- 0078 # ARM: dts: hpe-gxp: add node for GXP video thumbnail engine

### UDC (gadget)

- 0079 # dt-bindings: usb: add HPE USB Device Controllers binding
- 0080 # usb: gadget: udc: add HPE GXP USB device controller driver
- 0081 # ARM: dts: hpe-gxp: add USB UDC nodes

### UBM backplane 

- 0086 # misc: ubm: add minimal UBM backplane init driver

