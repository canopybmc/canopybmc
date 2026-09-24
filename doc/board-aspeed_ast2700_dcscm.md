# ASPEED AST2700 DC-SCM

## Supported Boards

| Silicon    | Machine          |
|------------|------------------|
| AST2700 A2 | `ast2700dcscm`   |
| AST2700 A1 | `ast2700a1dcscm` |

Both are single-node, hostless DC-SCM machines. Host and chassis control,
host IPMI, SOL, KVM, and fan management are not installed.

## Build

```bash
source setup ast2700dcscm # or ast2700a1dcscm
bitbake obmc-phosphor-image
```

Images are written to `build/<machine>/tmp/deploy/images/<machine>/`. The BMC
image is 64 MiB.

## Firmware

The ASPEED SDK layers supply the boot firmware, kernel, and image assembly; see
[vendor/README.md](../vendor/README.md). A1 uses `caliptra-fw.bin`; boards with
A1 ROM patch v1 require `CALIPTRA_FW_BINARY = "caliptra-fw-v1p1.bin"`.

## Signing

`ASPEED_CUSTOMIZE_GEN_SECURE_IMAGE_ENABLE = "0"` disables ASPEED secure-image
generation. The SDK ships public test keys only. Production images require
controlled keys and provisioning.
