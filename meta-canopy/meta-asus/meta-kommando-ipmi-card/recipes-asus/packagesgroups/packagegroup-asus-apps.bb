SUMMARY = "OpenBMC for Asus - applications"
PR = "r1"

inherit packagegroup

PROVIDES = "${PACKAGES}"
PACKAGES = " \
        ${PN}-chassis \
        ${PN}-flash \
        ${PN}-fans \
        ${PN}-system \
        "

PROVIDES += "virtual/obmc-chassis-mgmt"
PROVIDES += "virtual/obmc-fan-mgmt"
PROVIDES += "virtual/obmc-flash-mgmt"
PROVIDES += "virtual/obmc-system-mgmt"

RPROVIDES:${PN}-chassis += "virtual-obmc-chassis-mgmt"
RPROVIDES:${PN}-fans += "virtual-obmc-fan-mgmt"
RPROVIDES:${PN}-flash += "virtual-obmc-flash-mgmt"
RPROVIDES:${PN}-system += "virtual-obmc-system-mgmt"


SUMMARY:${PN}-flash = "Asus Flash"
RDEPENDS:${PN}-flash = " \
        phosphor-ipmi-flash \
        "

SUMMARY:${PN}-fans = "Asus Fans"
RDEPENDS:${PN}-fans = " \
        phosphor-pid-control \
        "

SUMMARY:${PN}-system = "Asus System"
RDEPENDS:${PN}-system = " \
        dbus-sensors \
        phosphor-host-postd \
        phosphor-post-code-manager \
        phosphor-software-manager \
        "