# COREBASE points at the openbmc submodule
OS_RELEASE_ROOTPATH = "${COREBASE}/.."

def run_git(d, cmd):
    try:
        oeroot = d.getVar('OS_RELEASE_ROOTPATH')
        return bb.process.run(
            'export PSEUDO_DISABLED=1; git --work-tree %s --git-dir %s/.git %s'
            % (oeroot, oeroot, cmd)
        )[0].strip('\n')
    except:
        pass

# Version schema:
#   Exact tag on HEAD: tag verbatim
#                        e.g. "2026.06", "2026.06-rc1", "2026.06-beta", "2026.03.1"
#   Untagged commit:   "<nearest-tag>-<commits-ahead>-<shorthash>"
#
#   The CANOPY_CUSTOM_TAG variable can be set to add a suffix.
#
# CANOPY_VERSION can be set in local.conf to hard-pin the base version and
# bypass all git detection.
python() {
    channel = d.getVar('CANOPY_CONFIG') or 'release'

    base = (d.getVar('CANOPY_VERSION') or '').strip()
    if not base:
        tag = run_git(d, 'describe --tags --exact-match HEAD')
        if tag:
            base = tag
        else:
            desc = run_git(d, 'describe --tags --long')
            if desc:
                parts = desc.rsplit('-', 2)
                base = '{}-{}-{}'.format(parts[0], parts[1], parts[2].lstrip('g'))
            else:
                bb.fatal(
                    'Canopy: no git tags found in the repository.\n'
                    'Please create an initial tag (e.g. "git tag 2026.06") or set\n'
                    'CANOPY_VERSION in local.conf to bypass git version detection.'
                )

    version = base

    custom_tag = (d.getVar('CANOPY_CUSTOM_TAG') or '').strip()
    if custom_tag:
        version = '{}-{}'.format(version, custom_tag)

    d.setVar('DISTRO_VERSION', version)
    d.setVar('EXTENDED_VERSION', version)
}

python do_compile:prepend() {
    if (d.getVar('CANOPY_CONFIG') or 'release') == 'dev':
        bb.warn(
            '\n'
            '  Canopy: building in dev mode (CANOPY_CONFIG = "dev")\n'
            '  Version string : {}\n'
            '  This may result in additional development tooling being installed\n'
            '  in the image.\n'
            '  Set CANOPY_CONFIG = "release" in conf/local.conf for production builds.\n'
            .format(d.getVar('EXTENDED_VERSION'))
        )
}
