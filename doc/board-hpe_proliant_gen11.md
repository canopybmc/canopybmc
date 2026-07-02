# HPE ProLiant Gen11

## Supported Boards

The HPE ProLiant Gen11 family consists of several variants. This list shows
all boards that are supported by the code and also lists (code) names
used throughout the code or elsewhere in the vendor documentation.

| Model                | Short    | SYSID | Validated |
|----------------------|----------|-------|-----------|
| ProLiant DL110 Gen11 | dl110g11 | U62   |    ❌     |
| ProLiant DL145 Gen11 | dl145g11 | A58   |    ❌     |
| ProLiant DL320 Gen11 | dl320g11 | U63   |    ✔️     |
| ProLiant DL345 Gen11 | dl345g11 | A56   |    ⚠️     |
| ProLiant DL360 Gen11 | dl360g11 | U54   |    ✔️     |
| ProLiant DL365 Gen11 | dl365g11 | A55   |    ✔️     |
| ProLiant DL380 Gen11 | dl380g11 | U54   |    ⚠️     |
| ProLiant DL385 Gen11 | dl385g11 | A55   |    ⚠️     |

**Legend**

- ✔️: validated via CI or direct HW access
- ⚠️: validated indirectly via HPE or OSFCI
- ❌: not validated but supported in the code

<!-- Unsupported right now. Comment out but keep for knowledge base -->
<!-- | ProLiant ML110 Gen11 | ml110g11 | U63   | -->
<!-- | ProLiant DL350 Gen11 | dl350g11 | U54   | -->
<!-- | ProLiant DL560 Gen11 | dl560g11 | U59   | -->
<!-- | ProLiant RL300 Gen11 | rl300g11 | R11   | -->

## Testing (OSFCI)

HPE offers a public infrastructure on which the Canopy images (or OpenBMC in general)
can be run.

See [HPE OSFCI](vendor-hpe.md#osfci) for more information.

## Signing Key

To build a flashable image for HPE ProLiant Gen11 systems, you must provide the
private key of the key pair used during the Transfer of Ownership (ToO) process.
Set the `HPE_SIGNING_KEY` environment variable to the path of your private key
before building.
In case this is not provided the image will be signed by the default OSFCI key.

You have several options for this (similar to other variables)

1. `local.conf`
    - Add `HPE_SIGNING_KEY = "/path/to/your/private_key.pem"` to your `local.conf` in
      `<project root>/build/hpe-proliant-g11/conf/`

2. Export
```bash
export HPE_SIGNING_KEY=/path/to/your/private_key.pem
```

3. Inline
```bash
HPE_SIGNING_KEY=/path/to/your/private_key.pem bitbake obmc-phosphor-image
```

> [!NOTE]
> Options 2 and 3 require `HPE_SIGNING_KEY` to be in `BB_ENV_PASSTHROUGH_ADDITIONS`
> so that BitBake picks it up from the environment. Add this to your `.envrc` or
> shell profile:
> ```bash
> export BB_ENV_PASSTHROUGH_ADDITIONS="$BB_ENV_PASSTHROUGH_ADDITIONS HPE_SIGNING_KEY"
> ```

## GXP Bootblock Selection

After a successful build, the `build/hpe-proliant-g11/tmp/deploy/images/hpe-proliant-g11/`
directory will contain multiple firmware images with different GXP bootblock variants.
Select the appropriate image for your target hardware:

| Image | Target Systems |
|-------|----------------|
| `obmc-phosphor-image-hpe-proliant-g11.GXP2loader-t26x-sgn00.static.mtd` | RL300 systems |
| `obmc-phosphor-image-hpe-proliant-g11.GXP2loader-t277-t280-t285-sgn00.static.mtd` | DL32x - DL38x systems (\*) |
| `obmc-phosphor-image-hpe-proliant-g11.GXP2loader-t282-t288-sgn00.static.mtd` | DL32x - DL38x systems (\*) |

\* The exact mapping from bootblock to model and/or revision needs clarification.
See [issue #95](https://github.com/canopybmc/canopybmc/issues/95)
