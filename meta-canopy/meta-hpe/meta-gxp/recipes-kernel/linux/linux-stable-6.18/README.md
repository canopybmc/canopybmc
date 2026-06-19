# Patch Overview

## Pure fixes to upstream

- 0001 # spi: gxp: fix out-of-bounds access in memory-mapped read
- 0002 # spi: gxp: support addressed reads with dummy cycles
- 0003 # i2c: gxp: fix use of error pointer on syscon lookup failure
- 0004 # mtd: spi-nor: macronix: allow MX66L51235F probe without SFDP

## Changes to upstream

- 0005 # dt-bindings: arm: hpe: simplify to generic GXP compatible
- 0006 # ARM: dts: hpe-gxp: convert to standalone device tree

## Additions

- 0007 # ARM: dts: hpe-gxp: expand device tree for boot support

### virtual EEPROM

- 0008 # dt-bindings: nvmem: add HPE GXP virtual EEPROM binding
- 0009 # nvmem: add HPE GXP virtual EEPROM driver
- 0010 # ARM: dts: hpe-gxp: add NVMEM virtual EEPROM with MAC address cells

### UMAC ethernet controller

- 0011 # dt-bindings: net: add HPE GXP UMAC ethernet controller bindings
- 0012a # net: ethernet: add HPE GXP UMAC driver
    - TODO: fix commit message
- 0013 # ARM: dts: hpe-gxp: add UMAC ethernet and MDIO nodes

### GXP SOC

- 0014 # dt-bindings: soc: hpe: add GXP SoC subsystem bindings
- 0015a # soc: hpe: add GXP SoC infrastructure drivers
    - TODO: fix commit message
- 0016 # gpio: add HPE GXP GPIO controller driver

### power, temp, cooling

#### GXP fan control (change/addition)
- 0017 # dt-bindings: hwmon: hpe,gxp-fan-ctrl: use syscon phandles
- 0018a # hwmon: gxp-fan-ctrl: use syscon phandles for XREG and FN2
    - TODO: fix commit message

#### GXP host power controller (new/addition)
- 0019 # dt-bindings: soc: hpe: add GXP host power controller binding
- 0020 # soc: hpe: add GXP host power controller driver
- 0021 # ARM: dts: hpe-gxp: add SoC infrastructure and power control
    - TODO: only regarding power controller. so split out LED,…
- further addition
    - 0022 # soc: hpe: gxp-power-ctrl: implement ForceRestart as VPBTN power cycle
- ---
- squash
    - 0023 # soc: hpe: gxp-power-ctrl: run full prepare-boot sequence on probe
        - TODO: squash to 0020?
    - 0024 # soc: hpe: gxp-power-ctrl: use CSM SW_RESET for warm reset
        - TODO: squash to 0020?
    - 0025 # soc: hpe: gxp-power-ctrl: demote PGOOD deasserted message to dev_dbg
        - TODO: squash to 0020.
    - 0026 # soc: hpe: gxp-power-ctrl: use IRQF_SHARED for PGOOD interrupt
        - TODO: squash to 0020
    - 0045 # soc: hpe: gxp-power-ctrl: re-arm boot gate after PGOOD de-assertion
        - TODO: squash to 0020
    - 0067 # ARM: dts: hpe-gxp: rename PSU GPIO lines
        - TODO: squash to 0021 ARM: dts: hpe-gxp: add SoC infrastructure and power control
        - TODO: fix Signed-off-by

### Virtual UART

- 0027 # dt-bindings: serial: add HPE GXP Virtual UART binding
- 0028 # serial: 8250: add HPE GXP Virtual UART driver
- 0029 # ARM: dts: hpe-gxp: add Virtual UART node

### CHIF

- 0030 # dt-bindings: soc: hpe: add GXP CHIF binding
- 0031a # soc: hpe: add GXP CHIF driver
    - TODO: fix commit message
- 0032 # ARM: dts: hpe-gxp: add CHIF node

### CPLD host power supply

- 0033 # dt-bindings: regulator: add HPE GXP CPLD host power supply
- 0034 # regulator: gxp-cpld: add HPE GXP CPLD host power supply driver
- 0035 # dt-bindings: regulator: hpe,gxp-host-power-supply: add interrupts
- further addition
    - 0036 # regulator: gxp-cpld: fire notifier events on PGOOD transitions

#### HWMON

- 0037 # dt-bindings: hwmon: gxp-fan-ctrl: replace fn2 syscon with fan-supply
- 0038 # hwmon: gxp-fan-ctrl: use fan-supply regulator and fix sysfs semantics
    - 0043 # hwmon: gxp-fan-ctrl: expose fan_input reporting PWM duty cycle
- 0039 # dt-bindings: soc: hpe: gxp-fn2: allow regulator child nodes

#### needs CLPD & fan-supply (could be split)

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

- 0054 # peci: controller: gxp: manage PECI devices via regulator events
- 0055 # peci: controller: gxp: prevent overlapping transfers

- 0056 # peci: core: serialize device creation with per-controller mutex

#### In mainline but not backported to OpenBMC/Linux

- 0057 # peci: cpu: add Intel Emerald Rapids support
    - [Mainline](https://github.com/torvalds/linux/commit/906f25050add51f1a412ea37e618d8748f75e23a)
- 0058 # hwmon: (peci/cputemp) add Intel Emerald Rapids support
    - [Mainline](https://github.com/torvalds/linux/commit/a45b3ae40451542e3d6b37b8fba04e280cc8efa7)
- 0059 # hwmon: (peci/dimmtemp) add Intel Emerald Rapids platform support
    - [Mainline](https://github.com/torvalds/linux/commit/03c5ecc276fdc696ec469ee3a784726b809ecf26)

### Fan speed on BMC reboot/shutdown

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
- 0074 # gxp-i2c-passthrough: fire uevent after I2C passthrough enable
    - TODO: could be integrated in 0072?

### KCS

- 0075 # ipmi: kcs_bmc_gxp: port driver to Linux v6.18
    - TODO: split in multiple patches (like the other additions): DT bindings, driver, dts

### Video capture & UDC (could/should be split)

- 0076 # media: add GXP thumbnail video capture driver
- 0077 # ARM: dts: hpe-gxp: add video thumbnail and USB UDC nodes
- 0078a # usb: gadget: udc: add HPE GXP USB device controller driver
    - TODO: Fix commit message
- 0079 # usb: gadget: gxp-udc: add port watchdog for EHCI handoff

### UBM backplane 

- 0086 # misc: ubm: add minimal UBM backplane init driver

### ramoops

- 0087 # ARM: dts: hpe: gxp: enable ramoops

