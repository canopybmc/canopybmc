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

## Build System Requirements

### Supported Operating Systems

- Ubuntu 24.04
- Fedora 42
- Fedora 43

### Known Issues

There are known issues with the build environment on some systems. Please check the
[open environment bugs](https://github.com/canopybmc/canopybmc/issues?q=is%3Aissue+state%3Aopen+label%3Aenvironment+label%3Abug)
for more information.

## Board-Specific Information

- [HPE ProLiant Gen11](doc/board-hpe_proliant_gen11.md)
