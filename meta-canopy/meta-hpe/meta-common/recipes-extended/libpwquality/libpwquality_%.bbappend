# Remove cracklib as dependency, as it only provides a dictionary
# for a check that isn't active rn.
DEPENDS:remove = "cracklib"
EXTRA_OECONF:append = " --disable-cracklib-check"
