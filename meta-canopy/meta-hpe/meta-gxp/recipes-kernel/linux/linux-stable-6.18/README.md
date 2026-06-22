# Patch Overview

## Pure fixes to upstream

- 0001 # spi: gxp: fix out-of-bounds access in memory-mapped read
    - TODO: add Upstream-Status
- 0002 # spi: gxp: support addressed reads with dummy cycles
    - TODO: add Upstream-Status
- 0003 # i2c: gxp: fix use of error pointer on syscon lookup failure
    - TODO: add Upstream-Status
- 0004 # mtd: spi-nor: macronix: allow MX66L51235F probe without SFDP
    - TODO: add Upstream-Status

## Changes to upstream

- 0005 # dt-bindings: arm: hpe: simplify to generic GXP compatible
    - TODO: add Upstream-Status
- 0006 # ARM: dts: hpe-gxp: convert to standalone device tree
    - TODO: add Upstream-Status

## Additions

- 0007 # ARM: dts: hpe-gxp: expand device tree for boot support
    - TODO: add Upstream-Status

### virtual EEPROM

- 0008 # dt-bindings: nvmem: add HPE GXP virtual EEPROM binding
    - TODO: add Upstream-Status
- 0009 # nvmem: add HPE GXP virtual EEPROM driver
    - TODO: add Upstream-Status
- 0010 # ARM: dts: hpe-gxp: add NVMEM virtual EEPROM with MAC address cells
    - TODO: add Upstream-Status

### UMAC ethernet controller

- 0011 # dt-bindings: net: add HPE GXP UMAC ethernet controller bindings
    - TODO: add Upstream-Status
- 0012a # net: ethernet: add HPE GXP UMAC driver
    - TODO: fix commit message
    - TODO: add Upstream-Status
- 0013 # ARM: dts: hpe-gxp: add UMAC ethernet and MDIO nodes
    - TODO: add Upstream-Status

### GXP SOC

- 0014 # dt-bindings: soc: hpe: add GXP SoC subsystem bindings
    - TODO: add Upstream-Status
- 0015a # soc: hpe: add GXP SoC infrastructure drivers
    - TODO: fix commit message
    - TODO: add Upstream-Status
- 0016 # gpio: add HPE GXP GPIO controller driver
    - TODO: add Upstream-Status

### power, temp, cooling

#### GXP fan control (change/addition)
- 0017 # dt-bindings: hwmon: hpe,gxp-fan-ctrl: use syscon phandles
    - TODO: add Upstream-Status
- 0018a # hwmon: gxp-fan-ctrl: use syscon phandles for XREG and FN2
    - TODO: fix commit message
    - TODO: add Upstream-Status

#### GXP host power controller (new/addition)

- 0019 # dt-bindings: soc: hpe: add GXP host power controller binding
    - TODO: add Upstream-Status
- 0020 # soc: hpe: add GXP host power controller driver
    - TODO: add Upstream-Status
- 0021 # ARM: dts: hpe-gxp: add SoC infrastructure and power control
    - TODO: only regarding power controller. so split out LED,…
    - TODO: add Upstream-Status
- further addition
    - 0022 # soc: hpe: gxp-power-ctrl: implement ForceRestart as VPBTN power cycle
        - TODO: add Upstream-Status
- ---
- squash
    - 0023 # soc: hpe: gxp-power-ctrl: run full prepare-boot sequence on probe
        - TODO: squash to 0020? | add Upstream-Status
    - 0024 # soc: hpe: gxp-power-ctrl: use CSM SW_RESET for warm reset
        - TODO: squash to 0020? | add Upstream-Status
    - 0025 # soc: hpe: gxp-power-ctrl: demote PGOOD deasserted message to dev_dbg
        - TODO: squash to 0020. | add Upstream-Status
    - 0026 # soc: hpe: gxp-power-ctrl: use IRQF_SHARED for PGOOD interrupt
        - TODO: squash to 0020 | add Upstream-Status
    - 0045 # soc: hpe: gxp-power-ctrl: re-arm boot gate after PGOOD de-assertion
        - TODO: squash to 0020 | add Upstream-Status
    - 0067 # ARM: dts: hpe-gxp: rename PSU GPIO lines
        - TODO: squash to 0021 ARM: dts: hpe-gxp: add SoC infrastructure and power control
          | add Upstream-Status
        - TODO: fix Signed-off-by

### Virtual UART

- 0027 # dt-bindings: serial: add HPE GXP Virtual UART binding
    - TODO: add Upstream-Status
- 0028 # serial: 8250: add HPE GXP Virtual UART driver
    - TODO: add Upstream-Status
- 0029 # ARM: dts: hpe-gxp: add Virtual UART node
    - TODO: add Upstream-Status

### CHIF

- 0030 # dt-bindings: soc: hpe: add GXP CHIF binding
    - TODO: add Upstream-Status
- 0031a # soc: hpe: add GXP CHIF driver
    - TODO: fix commit message
    - TODO: add Upstream-Status
- 0032 # ARM: dts: hpe-gxp: add CHIF node
    - TODO: add Upstream-Status

### CPLD host power supply

- 0033 # dt-bindings: regulator: add HPE GXP CPLD host power supply
    - TODO: add Upstream-Status
- 0034 # regulator: gxp-cpld: add HPE GXP CPLD host power supply driver
    - TODO: add Upstream-Status
- 0035 # dt-bindings: regulator: hpe,gxp-host-power-supply: add interrupts
    - TODO: add Upstream-Status
- further addition
    - 0036 # regulator: gxp-cpld: fire notifier events on PGOOD transitions
        - TODO: add Upstream-Status

#### HWMON fan control

- 0037 # dt-bindings: hwmon: gxp-fan-ctrl: replace fn2 syscon with fan-supply
    - TODO: add Upstream-Status
- 0038 # hwmon: gxp-fan-ctrl: use fan-supply regulator and fix sysfs semantics
    - TODO: add Upstream-Status
    - 0043 # hwmon: gxp-fan-ctrl: expose fan_input reporting PWM duty cycle
- 0039 # dt-bindings: soc: hpe: gxp-fn2: allow regulator child nodes
    - TODO: add Upstream-Status

#### needs CLPD & fan-supply (not directly related)

- 0040 # ARM: dts: hpe-gxp: add CPLD host power regulator and fan-supply
    - TODO: add Upstream-Status
- 0041 # ARM: dts: hpe-gxp: add PGOOD interrupt to host power regulator
    - TODO: add Upstream-Status
- 0042 # regulator: gxp-cpld: debounce PGOOD IRQ before notifying consumers
    - TODO: add Upstream-Status

- 0044 # ARM: dts: hpe-gxp: add gpio-keys-polled node for fan presence
    - TODO: add Upstream-Status

### SOC temp

- 0046 # dt-bindings: hwmon: add HPE GXP SoC temperature sensor binding
    - TODO: add Upstream-Status
- 0047 # hwmon: gxp-coretemp: add HPE GXP SoC temperature sensor driver
    - TODO: add Upstream-Status
- 0048 # ARM: dts: hpe-gxp: add coretemp sensor node
    - TODO: add Upstream-Status

### PECI

- 0049 # peci: core: export device lifecycle helpers for controller drivers
    - TODO: add Upstream-Status
- 0050 # peci: request: retry on unrecognized completion codes
    - TODO: add Upstream-Status
- 0051 # dt-bindings: peci: add HPE GXP PECI controller binding
    - TODO: add Upstream-Status
- 0052 # peci: controller: add HPE GXP PECI controller driver
    - TODO: add Upstream-Status
- 0053 # ARM: dts: hpe-gxp: add PECI controller node
    - TODO: add Upstream-Status

- 0054 # peci: controller: gxp: manage PECI devices via regulator events
    - TODO: add Upstream-Status
- 0055 # peci: controller: gxp: prevent overlapping transfers
    - TODO: ? squash to 0052 peci: controller: add HPE GXP PECI controller driver
      | add Upstream-Status

- 0056 # peci: core: serialize device creation with per-controller mutex
    - TODO: add Upstream-Status

#### In mainline but not backported to OpenBMC/Linux

- 0057 # peci: cpu: add Intel Emerald Rapids support
    - [Mainline](https://github.com/torvalds/linux/commit/906f25050add51f1a412ea37e618d8748f75e23a)
    - TODO: add Upstream-Status
- 0058 # hwmon: (peci/cputemp) add Intel Emerald Rapids support
    - [Mainline](https://github.com/torvalds/linux/commit/a45b3ae40451542e3d6b37b8fba04e280cc8efa7)
    - TODO: add Upstream-Status
- 0059 # hwmon: (peci/dimmtemp) add Intel Emerald Rapids platform support
    - [Mainline](https://github.com/torvalds/linux/commit/03c5ecc276fdc696ec469ee3a784726b809ecf26)
    - TODO: add Upstream-Status

### Fan speed on BMC reboot/shutdown/kernel panic

- 0060 # dt-bindings: hwmon: gxp-fan-ctrl: add fan-shutdown-percent property
    - TODO: add Upstream-Status
- 0061 # hwmon: gxp-fan-ctrl: restore safe fan speed on shutdown and removal
    - TODO: add Upstream-Status
- 0062 # ARM: dts: hpe-gxp: set fan shutdown speed to 50 percent
    - TODO: add Upstream-Status
- 0063 # hwmon: gxp-fan-ctrl: restore fan speed on kernel panic
    - TODO: add Upstream-Status

### unrelated? undecided

- 0065 # ARM: dts: hpe-gxp: add mmio-mux for i2c bus mux select registers

### GXP PSU

- 0066 # hwmon: add HPE GXP PSU driver
    - TODO: add Signed-off-by by original (HPE) authors

### regulators?

- 0068 # hwmon: sbtsi_temp: add regulator supply and probe deferral support
    - TODO: add Upstream-Status
- 0069 # misc: sbrmi: add regulator supply support
    - TODO: add Upstream-Status
- 0070 # misc: amd-sbi: add SB-RMI revision 0x21 (Turin) protocol support
    - TODO: add Upstream-Status

### I2C passthrough

- 0071 # dt-bindings: soc: hpe: add GXP I2C passthrough binding
- 0072 # soc: hpe: add GXP I2C passthrough driver
- 0073 # ARM: dts: hpe-gxp: add I2C passthrough node
- 0074 # gxp-i2c-passthrough: fire uevent after I2C passthrough enable
    - TODO: could be integrated in 0072? | add Upstream-Status

### KCS

- 0075 # ipmi: kcs_bmc_gxp: port driver to Linux v6.18
    - TODO: split in multiple patches (like the other additions): DT bindings, driver, dts
    - TODO: add Signed-off-by by original (HPE) authors
    - TODO: add Upstream-Status

### Video capture & UDC (could/should be split)

- 0076 # media: add GXP thumbnail video capture driver
- 0077 # ARM: dts: hpe-gxp: add video thumbnail and USB UDC nodes
- 0078a # usb: gadget: udc: add HPE GXP USB device controller driver
    - TODO: Fix commit message
- 0079 # usb: gadget: gxp-udc: add port watchdog for EHCI handoff
    - TODO: add Upstream-Status

### UBM backplane 

- 0086 # misc: ubm: add minimal UBM backplane init driver
    - TODO: add Upstream-Status

### ramoops

- 0087 # ARM: dts: hpe: gxp: enable ramoops
    - TODO: add Upstream-Status

