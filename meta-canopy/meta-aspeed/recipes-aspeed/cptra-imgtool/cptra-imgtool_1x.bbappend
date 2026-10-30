inherit perlnative

export PERL = "perl"

# The pinned openssl-sys needs its bundled OpenSSL; cargo_common forces
# the system one, which is a major version newer.
oe_cargo_fix_env:append() {
	unset OPENSSL_NO_VENDOR
}
