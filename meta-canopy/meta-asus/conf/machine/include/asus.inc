MACHINE_FEATURES += "\
    obmc-phosphor-chassis-mgmt \
    obmc-phosphor-fan-mgmt \
    obmc-phosphor-flash-mgmt \
    obmc-host-state-mgmt \
    obmc-chassis-state-mgmt \
    obmc-bmc-state-mgmt \
    obmc-host-ipmi\
    "

VIRTUAL-RUNTIME_obmc-inventory-manager = "entity-manager"
PREFERRED_PROVIDER_virtual/obmc-inventory-data = "entity-manager"

VIRTUAL-RUNTIME_obmc-host-state-manager ?= "x86-power-control"
VIRTUAL-RUNTIME_obmc-chassis-state-manager ?= "x86-power-control"
VIRTUAL-RUNTIME_obmc-sensors-hwmon = "dbus-sensors"
VIRTUAL-RUNTIME_obmc-fan-control = "phosphor-pid-control"

PREFERRED_PROVIDER_virtual/obmc-chassis-mgmt = "packagegroup-asus-apps"
PREFERRED_PROVIDER_virtual/obmc-fan-mgmt = "packagegroup-asus-apps"
PREFERRED_PROVIDER_virtual/obmc-flash-mgmt = "packagegroup-asus-apps"
PREFERRED_PROVIDER_virtual/obmc-system-mgmt = "packagegroup-asus-apps"

PREFERRED_PROVIDER_virtual/obmc-host-ipmi-hw ?= "phosphor-ipmi-kcs"