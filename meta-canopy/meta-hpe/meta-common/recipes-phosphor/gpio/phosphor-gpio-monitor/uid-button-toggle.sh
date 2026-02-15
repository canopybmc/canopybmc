#!/bin/sh
set -e
led_service=$(mapper get-service /xyz/openbmc_project/led/groups/enclosure_identify)
uid_state=$(busctl get-property "${led_service}" /xyz/openbmc_project/led/groups/enclosure_identify xyz.openbmc_project.Led.Group Asserted | cut -d " " -f 2)
if [ "${uid_state}" = "true" ]; then
    busctl set-property "${led_service}" /xyz/openbmc_project/led/groups/enclosure_identify xyz.openbmc_project.Led.Group Asserted b false
else
    busctl set-property "${led_service}" /xyz/openbmc_project/led/groups/enclosure_identify xyz.openbmc_project.Led.Group Asserted b true
fi
