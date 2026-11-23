FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-Reject-pre-authentication-payloads-during-session-se.patch \
    file://0002-Require-an-active-session-to-dispatch-commands.patch \
    file://0003-Defer-session-privilege-commitment-until-RAKP3-is-ve.patch \
    file://0004-rakp-prevent-RAKP1-relabeling-of-active-sessions.patch \
    file://0005-rakp-reject-RAKP3-against-stale-or-default-key-mater.patch \
"
