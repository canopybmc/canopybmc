IMAGE_CLASSES:append:ast2700-a1-dcscm = " image_types_phosphor_aspeed_g7"

IMAGE_FEATURES:remove:ast2700-a1-dcscm = " \
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
