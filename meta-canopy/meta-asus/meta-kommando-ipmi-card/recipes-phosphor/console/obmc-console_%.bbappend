FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://80-obmc-console-uart.rules \
    file://server.ttyVUART0.conf \
    file://client.2200.conf \
"
OBMC_CONSOLE_TTYS = "ttyVUART0"

do_install:append() {
    install -d ${D}${nonarch_base_libdir}/udev/rules.d
    install -m 0644 ${UNPACKDIR}/80-obmc-console-uart.rules \
        ${D}${nonarch_base_libdir}/udev/rules.d/
}
