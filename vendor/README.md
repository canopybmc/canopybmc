# Vendored layers

Layers used by the AST2700 DC-SCM machines that are not part of the `openbmc`
submodule.

## meta-aspeed-sdk

AST2700 A1/A2 BSP subset of the ASPEED OpenBMC SDK. Retains the firmware
boot chain, image generation and runtime integration; omits vendor application
bundles, unused utilities, emulator recipes and signing assets for other SoCs.

- Source: https://github.com/AspeedTech-BMC/openbmc
- Commit: `59db970dc4ed772070580a18b4c6774029d32110`
- Subtree: `meta-aspeed-sdk/`

## meta-zephyr

Upstream submodule pinned by the repository gitlink. Provides the Zephyr build
machinery; must retain `recipes-kernel/zephyr-kernel/zephyr-kernel-src-3.7.0.inc`.
