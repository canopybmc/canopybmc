# Canopy WebUI customization
# Activates --mode canopy which loads .env.canopy and _canopy.scss
# for Canopy branding (purple theme, Canopy logos, Inter font).
#
# All customization files are overlaid into the upstream webui-vue
# source tree at build time via do_configure:prepend, so the
# upstream repo stays untouched.

# Resolve the overlay directory at parse time
CANOPY_WEBUI_OVERLAYS := "${THISDIR}/${BPN}"

EXTRA_OENPM = "-- --mode canopy"

do_configure:prepend() {
    # Overlay Canopy customization files into the source tree
    install -m 0644 ${CANOPY_WEBUI_OVERLAYS}/dot-env.canopy ${S}/.env.canopy

    install -d ${S}/src/env/assets/styles
    install -m 0644 ${CANOPY_WEBUI_OVERLAYS}/_canopy.scss ${S}/src/env/assets/styles/_canopy.scss

    install -d ${S}/src/assets/images
    install -m 0644 ${CANOPY_WEBUI_OVERLAYS}/login-company-logo.svg ${S}/src/assets/images/login-company-logo.svg
    install -m 0644 ${CANOPY_WEBUI_OVERLAYS}/logo-header.svg ${S}/src/assets/images/logo-header.svg
    install -m 0644 ${CANOPY_WEBUI_OVERLAYS}/built-on-openbmc-logo.svg ${S}/src/assets/images/built-on-openbmc-logo.svg

    install -d ${S}/public
    install -m 0644 ${CANOPY_WEBUI_OVERLAYS}/favicon.ico ${S}/public/favicon.ico
}
