# Canopy WebUI customization
# Installs canopy settings as .env.production so vite can pick it up.
# With the migration from vue-cli to vite, the "NODE_ENV" variable got
# deprecated in vite and should not be used anymore. Now, the "mode"
# that's being used for evaluation is not the node env, but the environment
# (e.g. IBM, Intel), which breaks gzip compression.
# Therefore, we must copy the dotenv file to .env.production to get a working
# theme.

# Activates --mode canopy which loads .env.canopy and _canopy.scss
# for Canopy branding (purple theme, Canopy logos, Inter font).
#
# All customization files are overlaid into the upstream webui-vue
# source tree at build time via do_configure:prepend, so the
# upstream repo stays untouched.
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# Resolve the overlay directory at parse time
CANOPY_WEBUI_OVERLAYS := "${THISDIR}/${BPN}"

do_configure:prepend() {
    # Overlay Canopy customization files into the source tree
    install -m 0644 ${CANOPY_WEBUI_OVERLAYS}/dot-env.canopy ${S}/.env.production

    install -d ${S}/src/env/assets/styles
    install -m 0644 ${CANOPY_WEBUI_OVERLAYS}/_canopy.scss ${S}/src/env/assets/styles/_canopy.scss

    install -d ${S}/src/assets/images
    install -m 0644 ${CANOPY_WEBUI_OVERLAYS}/login-company-logo.svg ${S}/src/assets/images/login-company-logo.svg
    install -m 0644 ${CANOPY_WEBUI_OVERLAYS}/logo-header.svg ${S}/src/assets/images/logo-header.svg
    install -m 0644 ${CANOPY_WEBUI_OVERLAYS}/built-on-openbmc-logo.svg ${S}/src/assets/images/built-on-openbmc-logo.svg

    install -d ${S}/public
    install -m 0644 ${CANOPY_WEBUI_OVERLAYS}/favicon.ico ${S}/public/favicon.ico
}
