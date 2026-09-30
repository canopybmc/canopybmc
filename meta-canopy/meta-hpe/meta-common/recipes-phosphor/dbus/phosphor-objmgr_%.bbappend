FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-mappertool-make-inclusion-optional.patch \
    "

PACKAGECONFIG[mappertool] = "-Dmappertool=enabled,-Dmappertool=disabled"
