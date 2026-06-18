RDEPENDS:${PN}-inventory:append:kommando-ipmi-card = " phosphor-inventory-manager"
# TODO Remove PIM when https://gerrit.openbmc.org/q/topic:%22multi-host-fan-inventory%22 is merge
# EM doesnt support cooled_by association without this patch
