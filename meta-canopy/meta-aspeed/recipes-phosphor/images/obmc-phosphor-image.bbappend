IMAGE_CLASSES:append:canopy-ast2700-dcscm = " image_types_phosphor_aspeed_g7"

do_populate_lic_deploy[depends] += "${@oe.utils.conditional('ASPEED_CUSTOMIZE_GEN_SECURE_IMAGE_ENABLE', '1', 'aspeed-image-gen-secureboot:do_deploy', '', d)}"

IMAGE_FEATURES:remove:canopy-ast2700-dcscm = " \
    obmc-chassis-mgmt \
    obmc-chassis-state-mgmt \
    obmc-console \
    obmc-fan-control \
    obmc-fan-mgmt \
    obmc-flash-mgmt \
    obmc-host-ctl \
    obmc-host-ipmi \
    obmc-host-state-mgmt \
    obmc-ikvm \
    obmc-net-ipmi \
    obmc-system-mgmt \
    "
