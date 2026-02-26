project(
    'gxp-chif-svc',
    'cpp',
    version: '1.0.0',
    license: 'Apache-2.0',
    default_options: [
        'cpp_std=c++23',
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.63.0',
)

systemd_dep = dependency('systemd')
sdbusplus_dep = dependency('sdbusplus')
phosphor_dbus_dep = dependency('phosphor-dbus-interfaces')
phosphor_logging_dep = dependency('phosphor-logging')

systemd_system_unit_dir = systemd_dep.get_variable(
    'systemdsystemunitdir',
    pkgconfig_define: ['prefix', get_option('prefix')],
)

install_data(
    'service_files/xyz.openbmc_project.GxpChif.service',
    install_dir: systemd_system_unit_dir,
)

udev_dep = dependency('udev')
udev_rules_dir = udev_dep.get_variable(
    'udev_dir',
    pkgconfig_define: ['prefix', get_option('prefix')],
) / 'rules.d'

install_data(
    'service_files/90-gxp-chif.rules',
    install_dir: udev_rules_dir,
)

src_files = files(
    'src/main.cpp',
    'src/chif_daemon.cpp',
    'src/rom_service.cpp',
    'src/smbios_writer.cpp',
    'src/mdr_bridge.cpp',
)

executable(
    'chif',
    src_files,
    dependencies: [
        sdbusplus_dep,
        phosphor_dbus_dep,
        phosphor_logging_dep,
        systemd_dep,
    ],
    install: true,
    install_dir: get_option('bindir'),
)

if get_option('tests').enabled()
    gtest_dep = dependency('gtest', main: true)
    gtest_main_dep = dependency('gtest_main')

    test_src_common = files(
        'src/rom_service.cpp',
        'src/smbios_writer.cpp',
        'src/mdr_bridge.cpp',
        'src/chif_daemon.cpp',
    )

    test_packet = executable(
        'test_packet',
        'test/test_packet.cpp',
        test_src_common,
        dependencies: [
            gtest_dep,
            gtest_main_dep,
            sdbusplus_dep,
            phosphor_dbus_dep,
            phosphor_logging_dep,
            systemd_dep,
        ],
    )
    test('packet', test_packet)

    test_smbios_writer = executable(
        'test_smbios_writer',
        'test/test_smbios_writer.cpp',
        test_src_common,
        dependencies: [
            gtest_dep,
            gtest_main_dep,
            sdbusplus_dep,
            phosphor_dbus_dep,
            phosphor_logging_dep,
            systemd_dep,
        ],
    )
    test('smbios_writer', test_smbios_writer)

    test_rom_service = executable(
        'test_rom_service',
        'test/test_rom_service.cpp',
        test_src_common,
        dependencies: [
            gtest_dep,
            gtest_main_dep,
            sdbusplus_dep,
            phosphor_dbus_dep,
            phosphor_logging_dep,
            systemd_dep,
        ],
    )
    test('rom_service', test_rom_service)
endif
