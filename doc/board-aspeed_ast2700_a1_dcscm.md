# ASPEED AST2700 A1 DC-SCM

## Supported Boards

- `ast2700-a1-dcscm`: ASPEED's DC-SCM 2.x reference card with AST2700 A1
  silicon and an AST1060 PFR chip.

The machine is single-node and hostless. Host and chassis control, host IPMI,
SOL, KVM, fan management and BMC firmware update are not installed.

Only A1 silicon is supported. A2 needs different Caliptra firmware and boot
image composition.

## Build

```bash
source setup ast2700-a1-dcscm
bitbake obmc-phosphor-image
```

Images are written to `build/ast2700-a1-dcscm/tmp/deploy/images/ast2700-a1-dcscm/`.
`obmc-phosphor-image-ast2700-a1-dcscm.static.mtd` is the 64 MiB flash image.
The `recovery_*` images are for UART recovery boot.

## Firmware

The boot firmware, kernel and image assembly come from the vendored ASPEED SDK
layer; see [vendor/README.md](../vendor/README.md).

The U-Boot region of the flash image holds, in order:

- Caliptra firmware (`caliptra-fw.bin`) at 0, 128 KiB.
- The BootMCU FMC image (Zephyr `mcu-runtime`) at 128 KiB, 896 KiB.
- The Caliptra manifest image at 1 MiB, which carries the DDR and DP firmware,
  TF-A, OP-TEE, U-Boot and the SSP/TSP firmware.

Boards with A1 ROM patch v1 require
`CALIPTRA_FW_BINARY = "caliptra-fw-v1p1.bin"`.

## Boot

The AST1060 holds the BMC in reset and gates its boot flash chip select. The
BMC only boots once the AST1060 runs firmware that releases it.

## Signing

Secure boot and OTP image generation are not included. The SDK only ships
public test keys. Production images need controlled keys and provisioning.
