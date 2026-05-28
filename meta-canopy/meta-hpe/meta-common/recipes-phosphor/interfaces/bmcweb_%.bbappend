PACKAGECONFIG:append = " redfish-allow-deprecated-power-thermal"

EXTRA_OEMESON:append = " \
    -Dhttp-body-limit=64 \
"
