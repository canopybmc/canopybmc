IMAGE_FEATURES:remove = "obmc-ikvm"

do_generate_static:append() {
    _append_image(os.path.join(d.getVar('DEPLOY_DIR_IMAGE', True), 'gxp-section'),
                  int(d.getVar('FLASH_GXP_OFFSET', True)),
                  int(d.getVar('FLASH_GXP_END', True)))
}
do_generate_static[depends] += "gxp-section:do_deploy"

do_mk_static_symlinks:append() {
    ln -sf ${HPE_GXP_BOOTBLOCK_IMAGE} ${IMGDEPLOYDIR}/gxp-bootblock.bin
}
do_mk_static_symlinks[depends] += "gxp-bootblock:do_deploy"
