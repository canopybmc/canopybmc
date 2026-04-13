FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
SRC_URI += " file://obmc-ikvm.service "
SRC_URI += " file://create_usbhid.sh "
SRC_URI += " file://gxp-usbhid-rebind.sh "
SRC_URI += " file://gxp-usbhid-rebind.service "
SRC_URI += " file://0001-ikvm-use-libvncserver-threaded-mode.patch "
SRC_URI += " file://0002-ikvm-add-XRGB32-framebuffer-support.patch "

SYSTEMD_SERVICE:${PN} += " gxp-usbhid-rebind.service "

FILES:${PN} += " \
    ${systemd_system_unitdir}/obmc-ikvm.service \
    ${systemd_system_unitdir}/gxp-usbhid-rebind.service \
    ${bindir}/create_usbhid.sh \
    ${bindir}/gxp-usbhid-rebind.sh \
"

do_install:append () {
    install -D -m 0644 ${UNPACKDIR}/obmc-ikvm.service ${D}${systemd_system_unitdir}
    install -D -m 0644 ${UNPACKDIR}/gxp-usbhid-rebind.service ${D}${systemd_system_unitdir}
    install -D -m 0755 ${UNPACKDIR}/create_usbhid.sh ${D}${bindir}
    install -D -m 0755 ${UNPACKDIR}/gxp-usbhid-rebind.sh ${D}${bindir}
}
