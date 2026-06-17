FILESEXTRAPATHS:prepend := "${THISDIR}/linux-hpe:"
FILESEXTRAPATHS:prepend := "${THISDIR}/linux-stable-6.18:"

KBRANCH = "dev-6.18"
LINUX_VERSION = "6.18.35"
SRCREV = "cf42ecff238c6c385328a8d2a9b436f3944f02f8"

require linux-hpe.inc

require linux-gxp-flash-layout.inc
require linux-gxp-multi-dtb.inc

ERROR_QA:remove = "patch-status"

SRC_URI:append = " \
    file://common/defconfig \
    file://common/openbmc-common.cfg \
    file://common/kvm.cfg \
    file://common/pca954x.cfg \
    file://common/ubm.cfg \
    "

SRC_URI:append:canopy:release = " \
    file://release/production.cfg \
    "

SRC_URI:append:canopy:dev = " \
    file://dev/trace.cfg \
    "

SRC_URI:append = " \
    file://0001-spi-gxp-fix-out-of-bounds-access-in-memory-mapped-re.patch \
    file://0002-spi-gxp-support-addressed-reads-with-dummy-cycles.patch \
    file://0003-i2c-gxp-fix-use-of-error-pointer-on-syscon-lookup-fa.patch \
    file://0004-mtd-spi-nor-macronix-allow-MX66L51235F-probe-without.patch \
    file://0005-dt-bindings-arm-hpe-simplify-to-generic-GXP-compatib.patch \
    file://0006-ARM-dts-hpe-gxp-convert-to-standalone-device-tree.patch \
    file://0007-ARM-dts-hpe-gxp-expand-device-tree-for-boot-support.patch \
    file://0008-dt-bindings-nvmem-add-HPE-GXP-virtual-EEPROM-binding.patch \
    file://0009-nvmem-add-HPE-GXP-virtual-EEPROM-driver.patch \
    file://0010-ARM-dts-hpe-gxp-add-NVMEM-virtual-EEPROM-with-MAC-ad.patch \
    file://0011-dt-bindings-net-add-HPE-GXP-UMAC-ethernet-controller.patch \
    file://0012a-net-ethernet-add-HPE-GXP-UMAC-driver.patch \
    file://0013-ARM-dts-hpe-gxp-add-UMAC-ethernet-and-MDIO-nodes.patch \
    file://0014-dt-bindings-soc-hpe-add-GXP-SoC-subsystem-bindings.patch \
    file://0015a-soc-hpe-add-GXP-SoC-infrastructure-drivers.patch \
    file://0016-gpio-add-HPE-GXP-GPIO-controller-driver.patch \
    file://0017-dt-bindings-hwmon-hpe-gxp-fan-ctrl-use-syscon-phandl.patch \
    file://0018a-hwmon-gxp-fan-ctrl-use-syscon-phandles-for-XREG-and-.patch \
    file://0019-dt-bindings-soc-hpe-add-GXP-host-power-controller-bi.patch \
    file://0020-soc-hpe-add-GXP-host-power-controller-driver.patch \
    file://0021-ARM-dts-hpe-gxp-add-SoC-infrastructure-and-power-con.patch \
    file://0022-soc-hpe-gxp-power-ctrl-implement-ForceRestart-as-VPB.patch \
    file://0023-soc-hpe-gxp-power-ctrl-run-full-prepare-boot-sequenc.patch \
    file://0024-soc-hpe-gxp-power-ctrl-use-CSM-SW_RESET-for-warm-res.patch \
    file://0025-soc-hpe-gxp-power-ctrl-demote-PGOOD-deasserted-messa.patch \
    file://0026-soc-hpe-gxp-power-ctrl-use-IRQF_SHARED-for-PGOOD-int.patch \
    file://0027-dt-bindings-serial-add-HPE-GXP-Virtual-UART-binding.patch \
    file://0028-serial-8250-add-HPE-GXP-Virtual-UART-driver.patch \
    file://0029-ARM-dts-hpe-gxp-add-Virtual-UART-node.patch \
    file://0030-dt-bindings-soc-hpe-add-GXP-CHIF-binding.patch \
    file://0031a-soc-hpe-add-GXP-CHIF-driver.patch \
    file://0032-ARM-dts-hpe-gxp-add-CHIF-node.patch \
    file://0033-dt-bindings-regulator-add-HPE-GXP-CPLD-host-power-su.patch \
    file://0034-regulator-gxp-cpld-add-HPE-GXP-CPLD-host-power-suppl.patch \
    file://0035-dt-bindings-regulator-hpe-gxp-host-power-supply-add-.patch \
    file://0036-regulator-gxp-cpld-fire-notifier-events-on-PGOOD-tra.patch \
    file://0037-dt-bindings-hwmon-gxp-fan-ctrl-replace-fn2-syscon-wi.patch \
    file://0038-hwmon-gxp-fan-ctrl-use-fan-supply-regulator-and-fix-.patch \
    file://0039-dt-bindings-soc-hpe-gxp-fn2-allow-regulator-child-no.patch \
    file://0040-ARM-dts-hpe-gxp-add-CPLD-host-power-regulator-and-fa.patch \
    file://0041-ARM-dts-hpe-gxp-add-PGOOD-interrupt-to-host-power-re.patch \
    file://0042-regulator-gxp-cpld-debounce-PGOOD-IRQ-before-notifyi.patch \
    file://0043-hwmon-gxp-fan-ctrl-expose-fan_input-reporting-PWM-du.patch \
    file://0044-ARM-dts-hpe-gxp-add-gpio-keys-polled-node-for-fan-pr.patch \
    file://0045-soc-hpe-gxp-power-ctrl-re-arm-boot-gate-after-PGOOD-.patch \
    file://0046-dt-bindings-hwmon-add-HPE-GXP-SoC-temperature-sensor.patch \
    file://0047-hwmon-gxp-coretemp-add-HPE-GXP-SoC-temperature-senso.patch \
    file://0048-ARM-dts-hpe-gxp-add-coretemp-sensor-node.patch \
    file://0049-peci-core-export-device-lifecycle-helpers-for-contro.patch \
    file://0050-peci-request-retry-on-unrecognized-completion-codes.patch \
    file://0051-dt-bindings-peci-add-HPE-GXP-PECI-controller-binding.patch \
    file://0052-peci-controller-add-HPE-GXP-PECI-controller-driver.patch \
    file://0053-ARM-dts-hpe-gxp-add-PECI-controller-node.patch \
    file://0054-peci-controller-gxp-manage-PECI-devices-via-regulato.patch \
    file://0055-peci-controller-gxp-prevent-overlapping-transfers.patch \
    file://0056-peci-core-serialize-device-creation-with-per-control.patch \
    file://0057-peci-cpu-add-Intel-Emerald-Rapids-support.patch \
    file://0058-hwmon-peci-cputemp-add-Intel-Emerald-Rapids-support.patch \
    file://0059-hwmon-peci-dimmtemp-add-Intel-Emerald-Rapids-platfor.patch \
    file://0060-dt-bindings-hwmon-gxp-fan-ctrl-add-fan-shutdown-perc.patch \
    file://0061-hwmon-gxp-fan-ctrl-restore-safe-fan-speed-on-shutdow.patch \
    file://0062-ARM-dts-hpe-gxp-set-fan-shutdown-speed-to-50-percent.patch \
    file://0063-hwmon-gxp-fan-ctrl-restore-fan-speed-on-kernel-panic.patch \
    file://0065-ARM-dts-hpe-gxp-add-mmio-mux-for-i2c-bus-mux-select-.patch \
    file://0066-hwmon-add-HPE-GXP-PSU-driver.patch \
    file://0067-ARM-dts-hpe-gxp-rename-PSU-GPIO-lines.patch \
    file://0068-hwmon-sbtsi_temp-add-regulator-supply-and-probe-defe.patch \
    file://0069-misc-sbrmi-add-regulator-supply-support.patch \
    file://0070-misc-amd-sbi-add-SB-RMI-revision-0x21-Turin-protocol.patch \
    file://0071-dt-bindings-soc-hpe-add-GXP-I2C-passthrough-binding.patch \
    file://0072-soc-hpe-add-GXP-I2C-passthrough-driver.patch \
    file://0073-ARM-dts-hpe-gxp-add-I2C-passthrough-node.patch \
    file://0074-gxp-i2c-passthrough-fire-uevent-after-I2C-passthroug.patch \
    file://0075-ipmi-kcs_bmc_gxp-port-driver-to-Linux-v6.18.patch \
    file://0076-media-add-GXP-thumbnail-video-capture-driver.patch \
    file://0077-ARM-dts-hpe-gxp-add-video-thumbnail-and-USB-UDC-node.patch \
    file://0078a-usb-gadget-udc-add-HPE-GXP-USB-device-controller-dri.patch \
    file://0079-usb-gadget-gxp-udc-add-port-watchdog-for-EHCI-handof.patch \
    file://0086-misc-ubm-add-minimal-UBM-backplane-init-driver.patch \
    file://0087-ARM-dts-hpe-gxp-enable-ramoops.patch \
    "
