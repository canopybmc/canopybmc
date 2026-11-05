# The vendored SDK only keeps the AST2700 keys and OTP configs.
do_install() {
	install -d ${D}${datadir}/aspeed-secure-config/ast2700/otp
	install -d ${D}${datadir}/aspeed-secure-config/ast2700/keys
	install -m 0755 ${S}/configs/*.sh ${D}${datadir}/aspeed-secure-config
	install -m 0644 ${S}/keys/ast2700/* ${D}${datadir}/aspeed-secure-config/ast2700/keys
	install -m 0644 ${S}/configs/ast2700/otp/* ${D}${datadir}/aspeed-secure-config/ast2700/otp
}
