# Vendored layers

Layers used by the AST2700 A1 DC-SCM machine that are not part of the
`openbmc` submodule. Upstream OpenBMC has no AST2700 U-Boot, kernel defconfig
or BootMCU firmware recipe, and the AST2700 support under review there targets
A2 only, so the A1 boot chain comes from the ASPEED SDK.

## meta-aspeed-sdk

AST2700 A1 subset of the ASPEED OpenBMC SDK.

- Source: https://github.com/AspeedTech-BMC/openbmc
- Commit: `59db970dc4ed772070580a18b4c6774029d32110` (SDK v11.03)
- Subtree: `meta-aspeed-sdk/`

Files are only ever deleted from the SDK subtree, never edited, so that a later
re-sync stays a mechanical diff. Canopy changes go into `meta-canopy/meta-aspeed`.

Kept: the boot chain (bmc-pb prebuilts, Zephyr BootMCU and FMC image, Caliptra
manifest tooling, TF-A, OP-TEE and U-Boot 2023.10 appends, linux-aspeed 6.18),
the AST2700 image class, UART recovery images, and the udev, systemd watchdog
and volatile-binds integration.

Removed:

- Other SoCs and releases: everything outside AST2700 in the SDK, U-Boot
  2019.04 and linux-aspeed 5.15, 6.6 and 6.12.
- Providers not selected for A1: the bootmcu-spl recipe and its RISC-V
  toolchain, the prebuilt `ssp` and `tsp` recipes, FreeRTOS SSP/TSP and the
  Ibex firmware.
- Secure boot, iROT and AST27x5 support: secure-mode includes, socsec,
  `aspeed-image-gen-secureboot`, `aspeed-image-irot` and their image classes.
- eMMC and UFS boot: `ast-mmc.inc`, `ast-ufs.inc`, the wic files and
  `aspeed-image-emmcboot`.
- SDK userspace defaults: busybox, rsyslog, jsnbd, iperf3 and os-release
  appends.

## meta-zephyr

Upstream submodule pinned by the repository gitlink. Provides the Zephyr build
machinery for the BootMCU, SSP and TSP firmware; must retain
`recipes-kernel/zephyr-kernel/zephyr-kernel-src-3.7.0.inc`.

The pin matches the meta-zephyr import proposed for openbmc in Gerrit change
94909. Once openbmc carries it, point the layer template at
`openbmc/meta-zephyr` and remove this submodule.
