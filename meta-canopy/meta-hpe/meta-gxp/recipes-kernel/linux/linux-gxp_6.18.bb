KBRANCH ?= "hpe-gxp-g11"
LINUX_VERSION ?= "6.18.0"
KSRC = "git:///home/yocto/canopy-linux;branch=${KBRANCH}"
SRCREV = "6f6f070c24f96a08b2f15da11397d6a5048c4c60"
DEVICETREE_FLAGS += "-@"
KERNEL_DTC_FLAGS:append = " -@"

# note to self, add the remaining ones if we decide to increase initramfs size
KERNEL_DEVICETREE_OVERLAYS += " proliant_0x0250.dtbo proliant_0x0235.dtbo proliant_0x0236.dtbo proliant_0x0243.dtbo proliant_0x0244.dtbo proliant_0x0246.dtbo proliant_0x0248.dtbo"
# proliant_0x0249.dtbo proliant_0x0261.dtbo proliant_0x0263.dtbo proliant_0x0245.dtbo proliant_0x0271.dtbo proliant_0x0264.dtbo proliant_0x0273.dtbo"
KERNEL_DEVICETREE += " gxp_proliant_0x0250.dtb gxp_proliant_0x0235.dtb gxp_proliant_0x0236.dtb gxp_proliant_0x0243.dtb gxp_proliant_0x0244.dtb gxp_proliant_0x0246.dtb gxp_proliant_0x0248.dtb"
# gxp_proliant_0x0249.dtb gxp_proliant_0x0261.dtb gxp_proliant_0x0263.dtb gxp_proliant_0x0245.dtb gxp_proliant_0x0271.dtb gxp_proliant_0x0264.dtb gxp_proliant_0x0273.dtb"

require linux-gxp.inc
