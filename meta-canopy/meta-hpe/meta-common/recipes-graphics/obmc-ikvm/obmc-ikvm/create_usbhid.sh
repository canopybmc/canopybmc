#!/bin/sh

hid_conf_directory="/sys/kernel/config/usb_gadget/obmc_hid"
dev_name="80401000.usb"

create_hid() {
    # create gadget
    mkdir "${hid_conf_directory}"
    cd "${hid_conf_directory}" || exit 1

    # add basic information
    echo 0x0100 > bcdDevice
    echo 0x0200 > bcdUSB
    echo 0x0104 > idProduct		# Multifunction Composite Gadget
    echo 0x1d6b > idVendor		# Linux Foundation

    # create English locale
    mkdir strings/0x409

    echo "OpenBMC" > strings/0x409/manufacturer
    echo "virtual_input" > strings/0x409/product
    echo "OBMC0001" > strings/0x409/serialnumber

    # Create HID keyboard function
    mkdir functions/hid.0

    echo 1 > functions/hid.0/protocol	# 1: keyboard
    echo 8 > functions/hid.0/report_length
    echo 1 > functions/hid.0/subclass

    # Binary HID keyboard descriptor
    printf '\x05\x01\x09\x06\xa1\x01\x05\x07\x19\xe0\x29\xe7\x15\x00\x25\x01\x75\x01\x95\x08\x81\x02\x95\x01\x75\x08\x81\x03\x95\x05\x75\x01\x05\x08\x19\x01\x29\x05\x91\x02\x95\x01\x75\x03\x91\x03\x95\x06\x75\x08\x15\x00\x25\x65\x05\x07\x19\x00\x29\x65\x81\x00\xc0' > functions/hid.0/report_desc

    # Create HID mouse function
    mkdir functions/hid.1

    echo 2 > functions/hid.1/protocol	# 2: mouse
    echo 6 > functions/hid.1/report_length
    echo 1 > functions/hid.1/subclass

    # Binary HID mouse descriptor (absolute coordinate)
    printf '\x05\x01\x09\x02\xa1\x01\x09\x01\xa1\x00\x05\x09\x19\x01\x29\x03\x15\x00\x25\x01\x95\x03\x75\x01\x81\x02\x95\x01\x75\x05\x81\x03\x05\x01\x09\x30\x09\x31\x35\x00\x46\xff\x7f\x15\x00\x26\xff\x7f\x65\x11\x55\x00\x75\x10\x95\x02\x81\x02\x09\x38\x15\xff\x25\x01\x35\x00\x45\x00\x75\x08\x95\x01\x81\x06\xc0\xc0' > functions/hid.1/report_desc

    # Create configuration
    mkdir configs/c.1
    mkdir configs/c.1/strings/0x409

    echo 0xe0 > configs/c.1/bmAttributes
    echo 200 > configs/c.1/MaxPower
    echo "" > configs/c.1/strings/0x409/configuration

    # Link HID functions to configuration
    ln -s functions/hid.0 configs/c.1
    ln -s functions/hid.1 configs/c.1
}

connect_hid() {
    # UDC binding is handled by obmc-ikvm via the -u flag.
    # Writing here would conflict with obmc-ikvm's Input::connect().
    :
}

disconnect_hid() {
    if grep -q "${dev_name}" UDC; then
        echo "" > UDC
    fi
}

if [ ! -e "${hid_conf_directory}" ]; then
    create_hid
else
    cd "${hid_conf_directory}" || exit 1
fi

if [ "$1" = "connect" ]; then
    connect_hid
elif [ "$1" = "disconnect" ]; then
    disconnect_hid
else
    echo >&2 "Invalid option: $1. Use 'connect' or 'disconnect'."
    exit 1
fi
