# Replace dropbearkey.service with a version that derives key types from
# DROPBEAR_EXTRA_ARGS (-r flags in /etc/default/dropbear). This ensures
# key generation stays in sync with which keys the daemon loads at runtime,
# eliminating wasted keygen for unused key types.

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " file://generate-host-keys"

do_install:append() {
    install -d ${D}${libexecdir}/dropbear
    install -m 0755 ${UNPACKDIR}/generate-host-keys \
        ${D}${libexecdir}/dropbear/generate-host-keys

    # Poky's do_install sed only handles @BASE_BINDIR@, @BINDIR@, @SBINDIR@.
    # Process @LIBEXECDIR@ in our dropbearkey.service.
    sed -i -e 's,@LIBEXECDIR@,${libexecdir},g' \
        ${D}${systemd_system_unitdir}/dropbearkey.service
}
