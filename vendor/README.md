# Vendored layers

Layers used by the AST2700 A1 DC-SCM machine that are not part of the
`openbmc` submodule. Upstream OpenBMC has no AST2700 kernel defconfig or
BootMCU firmware recipe, and the AST2700 support under review there targets
A2 only, so the A1 boot chain comes from the ASPEED SDK. U-Boot is the
exception: the machine builds `openbmc/u-boot` from `meta-canopy/meta-aspeed`.

## meta-aspeed-sdk

AST2700 A1 subset of the ASPEED OpenBMC SDK.

- Source: https://github.com/AspeedTech-BMC/openbmc
- Commit: `6d993fae68f08115aff41e125a2c07b4bcece256` (SDK v11.04)
- Subtree: `meta-aspeed-sdk/`

Files are only ever deleted from the SDK subtree, never edited, so that a later
re-sync stays a mechanical diff. Canopy changes go into `meta-canopy/meta-aspeed`,
which also overrides what kept SDK files pull in:

- `aspeed-sdk.inc` selects the SDK U-Boot and fw-utils, so the machine selects
  `u-boot-aspeed` and `libubootenv` instead.
- `aspeed-sdk.inc` adds runqemu support, which the machine removes so the image
  doesn't build QEMU.
- `aspeed-secure-config.bb` installs the deleted AST2600 and AST2705 keys, so a
  bbappend replaces its `do_install` to install only the AST2700 ones. Check
  that recipe by hand on a re-sync.
- `linux-aspeed_6.18.bb` adds the deleted `ipmi_ssif.cfg` and `mtd_test.cfg`
  fragments, which the `linux-aspeed` append removes from `SRC_URI`.

Kept: the boot chain (bmc-pb prebuilts, Zephyr BootMCU and FMC image, Caliptra
manifest tooling, TF-A appends, linux-aspeed 6.18),
the AST2700 image class, and the udev and systemd watchdog integration.

Removed:

- Other SoCs and releases: everything outside AST2700 in the SDK and
  linux-aspeed 5.15, 6.6 and 6.12.
- U-Boot: the 2019.04 and 2023.10 recipes, fw-utils, their appends and
  environment files.
- Providers not selected for A1: the bootmcu-spl recipe and its RISC-V
  toolchain, the prebuilt `ssp` and `tsp` recipes, FreeRTOS SSP/TSP and the
  Ibex firmware.
- Secure boot, iROT and AST27x5 support: secure-mode includes, socsec,
  `aspeed-image-gen-secureboot`, `aspeed-image-irot` and their image classes.
- eMMC and UFS boot: `ast-mmc.inc`, `ast-ufs.inc`, the wic files and
  `aspeed-image-emmcboot`.
- UART recovery: `aspeed-image-recoveryuart` and `aspeed-image-tools`.
- OP-TEE debug extras: `optee-os-helper.inc` (commented-out log flags) and the
  `optee-os` and `optee-os-tadevkit` appends that only required it and deployed
  `tee.dmp` and `tee.map`.
- SDK userspace defaults: busybox, rsyslog, jsnbd, iperf3, os-release and
  volatile-binds appends.
- Kernel fragments: `ipmi_ssif.cfg`, which builds the host-side IPMI stack as
  modules, and `mtd_test.cfg`.
- Docs: the SDK's top-level `README.md`, which covers the SDK's own build setup.

## meta-zephyr

Upstream submodule pinned by the repository gitlink. Provides the Zephyr build
machinery for the BootMCU, SSP and TSP firmware; must retain
`recipes-kernel/zephyr-kernel/zephyr-kernel-src-3.7.0.inc`.

The pin matches the meta-zephyr import proposed for openbmc in Gerrit change
94909. Once openbmc carries it, point the layer template at
`openbmc/meta-zephyr` and remove this submodule.
