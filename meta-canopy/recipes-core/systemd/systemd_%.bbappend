# Remove once https://gerrit.openbmc.org/c/openbmc/openbmc/+/95968 is merged
EXTRA_OEMESON:append = " \
        -Dukify=disabled \
        -Dsysinstall=false \
        ${@bb.utils.contains('PACKAGECONFIG', 'repart', '', '-Dfdisk=disabled', d)} \
        -Dhwdb=false \
        "

do_install:append() {
    # Remove once https://gerrit.openbmc.org/c/openbmc/openbmc/+/95967 is merged
    rm ${D}${libdir}/udev/ata_id
    sed -i '/IMPORT{program}="ata_id /d' ${D}${libdir}/udev/rules.d/60-persistent-storage.rules

    # Remove once https://gerrit.openbmc.org/c/openbmc/openbmc/+/95968 is merged
    rm ${D}${bindir}/bootctl

    for f in systemd-storage-block systemd-storage-fs systemd-report \
             systemd-report-basic systemd-report-cgroup; do
        rm ${D}${nonarch_libdir}/systemd/${f}
    done
    rm ${D}${bindir}/storagectl ${D}${bindir}/systemd-mstack
    rm ${D}${base_sbindir}/mount.storage ${D}${base_sbindir}/mount.mstack
    for f in systemd-storage-block.socket systemd-storage-block@.service \
             systemd-storage-fs.socket systemd-storage-fs@.service \
             systemd-report-basic.socket systemd-report-basic@.service \
             systemd-report-cgroup.socket systemd-report-cgroup@.service \
             sockets.target.wants/systemd-storage-block.socket \
             sockets.target.wants/systemd-storage-fs.socket; do
        rm ${D}${systemd_system_unitdir}/${f}
    done
    for f in systemd-storage-fs.socket systemd-storage-fs@.service \
             sockets.target.wants/systemd-storage-fs.socket; do
        rm ${D}${systemd_user_unitdir}/${f}
    done
    rm ${D}${datadir}/polkit-1/actions/io.systemd.storage.policy
}
