# The host tools use the OpenSSL engine API, which OpenSSL 4 removed.
# Enable the stub API, as meta-arm does for its U-Boot.
BUILD_CFLAGS += "-DOPENSSL_ENGINE_STUBS"
