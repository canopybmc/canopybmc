IMAGE_FEATURES:remove = "obmc-ikvm"

# Append uboot-sig to the BMC image at the designated offset
do_generate_static:append() {
    _append_image(os.path.join(d.getVar('DEPLOY_DIR_IMAGE', True), 'gxp-uboot-sig'),
                  int(d.getVar('FLASH_UBOOT_SIG_OFFSET', True)),
                  int(d.getVar('FLASH_SIZE', True)))
}
do_generate_static[depends] += "gxp-uboot-sig:do_deploy"

# Generate provisioning images (BMC + GXP bootblock) and model symlinks
python do_generate_provisioning_images() {
    import subprocess, os

    imgdeploydir = d.getVar('IMGDEPLOYDIR', True)
    deploy_dir   = d.getVar('DEPLOY_DIR_IMAGE', True)
    image_name   = d.getVar('IMAGE_NAME', True)
    image_link   = d.getVar('IMAGE_LINK_NAME', True)
    bmc_image    = os.path.join(imgdeploydir, '%s.static.mtd' % image_name)
    gxp_offset   = int(d.getVar('FLASH_GXP_OFFSET', True))
    bb_size      = int(d.getVar('FLASH_GXP_BOOTBLOCK_SIZE', True))

    loaders = (d.getVar('HPE_GXP_LOADERS', True) or '').split()

    for loader in loaders:
        loader_bin = os.path.join(deploy_dir, '%s.bin' % loader)
        rom_name = '%s.%s.static.mtd' % (image_name, loader)
        rom_link = '%s.%s.static.mtd' % (image_link, loader)
        rom_path = os.path.join(imgdeploydir, rom_name)

        # Create ROM: copy BMC image, then append bootblock at GXP offset
        subprocess.check_call(['cp', bmc_image, rom_path])
        subprocess.check_call(['truncate', '-s', '%dK' % (gxp_offset + bb_size), rom_path])
        subprocess.check_call(['dd', 'bs=1k', 'conv=notrunc',
                               'seek=%d' % gxp_offset,
                               'if=%s' % loader_bin,
                               'of=%s' % rom_path])

        # Unversioned symlink for the ROM
        rom_link_path = os.path.join(imgdeploydir, rom_link)
        if os.path.lexists(rom_link_path):
            os.remove(rom_link_path)
        os.symlink(rom_name, rom_link_path)

        # Per-model symlinks
        models = (d.getVarFlag('HPE_GXP_LOADER_MODELS', loader) or '').split()
        for model in models:
            model_link = 'obmc-phosphor-image-hpe-proliant-%s.static.mtd' % model
            link_path = os.path.join(imgdeploydir, model_link)
            if os.path.lexists(link_path):
                os.remove(link_path)
            os.symlink(rom_name, link_path)

        bb.note("Created provisioning image: %s (models: %s)" %
                (rom_name, ', '.join(models) if models else 'none'))
}
do_generate_provisioning_images[depends] += "gxp-bootblock:do_deploy"
addtask generate_provisioning_images after do_generate_static before do_image_complete
