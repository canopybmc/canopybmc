IMAGE_FEATURES:remove = "obmc-ikvm"

FLASH_ROFS_OFFSET = "5376"
FLASH_RWFS_OFFSET = "30976"
FLASH_HPE_OFFSET = "32192"
FLASH_HPE_END = "32768"

do_generate_static:append() {
    _append_image(os.path.join(d.getVar('DEPLOY_DIR_IMAGE', True), 'hpe-section'),
                  int(d.getVar('FLASH_HPE_OFFSET', True)),
                  int(d.getVar('FLASH_HPE_END', True)))
}
do_generate_static[depends] += "hpe-section:do_deploy"
