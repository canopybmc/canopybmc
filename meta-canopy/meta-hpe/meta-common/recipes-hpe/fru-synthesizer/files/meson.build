project(
    'fru-synthesizer',
    'cpp',
    version: '1.0',
    default_options: [
        'cpp_std=c++23',
        'warning_level=3',
        'werror=false',
    ],
)

deps = [
    dependency('sdbusplus'),
    dependency('phosphor-logging'),
    dependency('boost'),
]

executable(
    'fru-synthesizer',
    ['main.cpp', 'fru_encoder.cpp'],
    dependencies: deps,
    install: true,
    install_dir: get_option('bindir'),
)

systemd = dependency('systemd')
systemd_system_unit_dir = systemd.get_variable(
    pkgconfig: 'systemdsystemunitdir',
)
install_data(
    'fru-synthesizer.service',
    install_dir: systemd_system_unit_dir,
)
