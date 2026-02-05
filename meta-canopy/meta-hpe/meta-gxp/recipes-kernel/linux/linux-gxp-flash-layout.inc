# Generate gxp-bmc-flash-layout.dtsi from the flash layout variables defined
# in gxp-flash-layout.inc. This keeps the kernel device tree partitions in
# sync with the image assembly offsets used by image_types_phosphor.

python do_gxp_generate_flash_layout() {
    import os

    def kb(v):
        return v * 1024

    def hx(v):
        return "0x%x" % v

    def var(name):
        return int(d.getVar(name))

    # All values in KB
    uboot_off     = var('FLASH_UBOOT_OFFSET')
    uboot_sz      = var('FLASH_UBOOT_SIZE')
    env_off       = var('FLASH_UBOOT_ENV_OFFSET')
    env_sz        = var('FLASH_UBOOT_ENV_SIZE')
    kernel_off    = var('FLASH_KERNEL_OFFSET')
    rofs_off      = var('FLASH_ROFS_OFFSET')
    rwfs_off      = var('FLASH_RWFS_OFFSET')
    sig_sz        = var('FLASH_UBOOT_SIG_SIZE')
    bootblock_sz  = var('FLASH_GXP_BOOTBLOCK_SIZE')
    ckb_sz        = var('FLASH_GXP_CKB_SIZE')

    # Derived offsets and sizes
    sig_off       = var('FLASH_UBOOT_SIG_OFFSET')
    bootblock_off = var('FLASH_GXP_OFFSET')
    ckb_off       = bootblock_off + bootblock_sz
    bmc_sz        = var('FLASH_SIZE')
    gxp_hw_sz     = bootblock_sz + ckb_sz

    kernel_sz     = rofs_off - kernel_off
    rofs_sz       = rwfs_off - rofs_off
    rwfs_sz       = sig_off - rwfs_off

    # Validate tunable partition ordering (fixed SoC partitions may
    # intentionally overlap and are not checked)
    if not (kernel_off < rofs_off < rwfs_off < sig_off):
        bb.fatal("Flash partition ordering violated: "
                 "kernel(%d) < rofs(%d) < rwfs(%d) < uboot-sig(%d)"
                 % (kernel_off, rofs_off, rwfs_off, sig_off))

    for name, size in [("kernel", kernel_sz), ("rofs", rofs_sz), ("rwfs", rwfs_sz)]:
        if size <= 0:
            bb.fatal("%s partition size is non-positive: %d KB" % (name, size))

    # (label, offset_kb, size_kb, extra properties)
    partitions = [
        ("bmc",           uboot_off,     bmc_sz,       {}),
        ("u-boot",        uboot_off,     uboot_sz,     {}),
        ("u-boot-env",    env_off,       env_sz,       {}),
        ("kernel",        kernel_off,    kernel_sz,     {}),
        ("rofs",          rofs_off,      rofs_sz,       {}),
        ("rwfs",          rwfs_off,      rwfs_sz,       {}),
        ("uboot-sig",     sig_off,       sig_sz,        {}),
        ("gxp",           bootblock_off, gxp_hw_sz,     {}),
        ("gxp-bootblock", bootblock_off, bootblock_sz,  {}),
        ("gxp-ckb",       ckb_off,       ckb_sz,        {"read-only": True}),
    ]

    lines = [
        "// SPDX-License-Identifier: GPL-2.0",
        "",
        "partitions {",
        '\tcompatible = "fixed-partitions";',
        "\t#address-cells = <1>;",
        "\t#size-cells = <1>;",
    ]

    for label, off_kb, sz_kb, props in partitions:
        off = hx(kb(off_kb))
        sz = hx(kb(sz_kb))
        lines.append("")
        lines.append("\t%s@%s {" % (label, off.replace("0x", "")))
        lines.append("\t\treg = <%s %s>;" % (off, sz))
        lines.append('\t\tlabel = "%s";' % label)
        if props.get("read-only"):
            lines.append("\t\tread-only;")
        lines.append("\t};")

    lines.append("};")
    lines.append("")

    dtsi_path = os.path.join(d.getVar('S'), 'arch', 'arm', 'boot', 'dts',
                             'hpe', 'gxp-bmc-flash-layout.dtsi')
    os.makedirs(os.path.dirname(dtsi_path), exist_ok=True)

    with open(dtsi_path, 'w') as f:
        f.write("\n".join(lines))

    bb.note("Generated %s" % dtsi_path)
}

addtask gxp_generate_flash_layout after do_patch before do_compile
do_gxp_generate_flash_layout[vardeps] += "\
    FLASH_UBOOT_OFFSET FLASH_KERNEL_OFFSET FLASH_ROFS_OFFSET \
    FLASH_RWFS_OFFSET FLASH_GXP_OFFSET FLASH_SIZE \
    FLASH_UBOOT_SIZE FLASH_UBOOT_ENV_OFFSET \
    FLASH_UBOOT_ENV_SIZE FLASH_UBOOT_SIG_SIZE \
    FLASH_UBOOT_SIG_OFFSET \
    FLASH_GXP_BOOTBLOCK_SIZE FLASH_GXP_CKB_SIZE \
"
