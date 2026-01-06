KBRANCH ?= "hpe-gxp-g11"
LINUX_VERSION ?= "6.18.0"
KSRC = "git://github.com/canopybmc/linux.git;branch=${KBRANCH};protocol=https"
SRCREV = "0a4fc82bf1df33c9949c8d9466bcb13e5264a366"
DEVICETREE_FLAGS += "-@"

require linux-gxp.inc
