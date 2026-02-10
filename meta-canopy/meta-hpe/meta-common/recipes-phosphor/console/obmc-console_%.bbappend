FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://80-gxp-obmc-console-uart.rules \
    file://server.ttyS0.conf \
    file://server.ttyVUART0.conf \
"

OBMC_CONSOLE_HOST_TTY = "ttyVUART0"
OBMC_CONSOLE_TTYS = "ttyS0 ttyVUART0"

do_install:append() {
    install -d ${D}${nonarch_base_libdir}/udev/rules.d
    install -m 0644 ${UNPACKDIR}/80-gxp-obmc-console-uart.rules \
        ${D}${nonarch_base_libdir}/udev/rules.d/
}
