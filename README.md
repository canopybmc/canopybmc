# Canopy BMC

This is the main repository of Canopy - a OpenBMC distribution. It provides a
customized OpenBMC build environment for supported server platforms.

> [!NOTE]
> The first release of this project (`2026.04`) constitutes a snapshot release. It focuses
> on HPE ProLiant Gen11 systems and is intended for early adopters and evaluation. The full
> release (`2026.06`) is intended for public adoption.

## Supported Boards

- `hpe-proliant-g11`

## Quick Start Guide

1. Clone the repository
```bash
git clone git@github.com:canopybmc/canopybmc.git
```

2. Initialize for build
```bash
source setup hpe-proliant-g11
```

3. Build
```bash
bitbake obmc-phosphor-image
```

## Host System Requirements

### Supported Operating Systems

- Ubuntu 24.04
- Fedora 42
- Fedora 43
- Arch Linux

### Known Issues

There are known issues with the build environment on some systems. Please check the
[open environment bugs](https://github.com/canopybmc/canopybmc/issues?q=is%3Aissue+state%3Aopen+label%3Aenvironment+label%3Abug)
for more information.

## Board-Specific Information

### HPE ProLiant G11

#### Signing Key

To build a flashable image for HPE ProLiant Gen11 systems, you must provide the
private key of the key pair used during the Transfer of Ownership (ToO) process.
Set the `HPE_SIGNING_KEY` environment variable to the path of your private key
before building:

```bash
export HPE_SIGNING_KEY=/path/to/your/private_key.pem
```

#### GXP Bootblock Selection

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
