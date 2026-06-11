FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# Drop the template-service Conflicts drop-in; we use a non-template service.
SYSTEMD_OVERRIDE:${PN}:remove = "poweron.conf:phosphor-watchdog@poweron.service.d/poweron.conf"

SYSTEMD_SERVICE:${PN} = " \
    phosphor-watchdog.service \
    phosphor-watchdog-host-poweroff.service \
    phosphor-watchdog-host-reset.service \
    phosphor-watchdog-host-powercycle.service \
    "
