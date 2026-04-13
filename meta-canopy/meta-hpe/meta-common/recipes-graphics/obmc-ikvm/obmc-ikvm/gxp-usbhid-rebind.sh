#!/bin/sh
#
# Refresh the USB HID gadget binding when the host state changes so the
# host OS sees a fresh USB attach event. Works around an HPE iLO
# internal USB hub quirk where the host does not get a clean
# disconnect/connect notification when the BMC's gadget binding changes
# silently. Without this, keyboard/mouse input works in BIOS/GRUB (which
# enumerated USB during POST) but stops working once Linux boots and
# inherits the stale enumeration state.
#
# Polls xyz.openbmc_project.State.Host CurrentHostState every 2 seconds.
# On any transition into the "Running" state, performs an unbind/rebind
# cycle on the configured UDC.

set -u

UDC_FILE=/sys/kernel/config/usb_gadget/obmc_hid/UDC
HOST_SERVICE=xyz.openbmc_project.State.Host
HOST_PATH=/xyz/openbmc_project/state/host0
HOST_IFACE=xyz.openbmc_project.State.Host

prev_state=""

while true; do
    state=$(busctl get-property "$HOST_SERVICE" "$HOST_PATH" \
            "$HOST_IFACE" CurrentHostState 2>/dev/null \
            | awk -F'"' '/"/{print $2}')

    if [ -n "$state" ] && [ "$state" != "$prev_state" ]; then
        case "$state" in
            *.Running)
                # Wait briefly for the host OS USB stack to be ready
                sleep 2

                # Only refresh if the UDC is currently bound — the
                # initial bind is owned by obmc-ikvm's Input::connect()
                current_udc=$(cat "$UDC_FILE" 2>/dev/null)
                if [ -n "$current_udc" ]; then
                    echo "" > "$UDC_FILE"
                    sleep 1
                    echo "$current_udc" > "$UDC_FILE"
                    echo "gxp-usbhid-rebind: refreshed UDC '$current_udc'" \
                        "after host transition to Running" >&2
                fi
                ;;
        esac
        prev_state="$state"
    fi
    sleep 2
done
