#!/bin/bash

getdevice() {
	#based on https://unix.stackexchange.com/questions/136781/how-to-get-device-filename-from-lsusb-output-or-by-device-path
	idV=${1%:*}
	idP=${1#*:}
	for path in $(dirname $(find -L /sys/bus/usb/devices/ -maxdepth 2 -name "idVendor")); do
	if grep -q $idV $path/idVendor && grep -q $idP $path/idProduct; then
			echo $path
		fi
	done
}

print_help() {
	cat << EOF
To switch the SD card to USB run:
$0 1
To switch the SD card to SBC run:
$0 0
If you have multiple devices run either of the above.
Then run with the last param the same as above:
$0 \$(Your USB number) 0

Examples:
# $0 0
Uisng device /sys/bus/usb/devices/3-4

# $0 0
Uisng device /sys/bus/usb/devices/3-4
Already off.

# $0 0
Multiple devices found! Pass a usb number as param 1...
Usb num: 3-4
Usb num: 3-1

# $0 3-1 0
Uisng device /sys/bus/usb/devices/3-1
EOF
	exit 1
}

if [ $# -eq 0 ] || ([ $(eval echo \${$#}) != '1' ] && [ $(eval echo \${$#}) != '0' ]); then
	print_help
fi

export DEVICES=$(getdevice 0bda:0316)
if echo $DEVICES | grep -q " "; then
	if [ $# -eq 2 ]; then
		for dev in $DEVICES; do
			if echo $dev | grep -q $1; then
				export DEVICE=$dev
			fi
		done
		if [ -z $DEVICE ]; then
			echo "Device not found. Pass a usb number as param 1..."
			for dev in $DEVICES; do
				echo "Usb num:" $(basename $dev)
			done
			exit 1
		fi
	else
		echo "Multiple devices found! Pass a usb number as param 1..."
		for dev in $DEVICES; do
			echo "Usb num:" $(basename $dev)
		done
		exit 1
	fi
else
		export DEVICE=$DEVICES
fi

if [ -z "$DEVICES" ]; then
	echo "No devices found ;("
	exit 1
fi

echo "Uisng device" $DEVICE
export USB_NUM=$(basename $DEVICE)

echo 0 | sudo tee ${DEVICE}/power/autosuspend_delay_ms &>/dev/null
echo auto | sudo tee ${DEVICE}/power/control &>/dev/null

if [ $(eval echo \${$#}) == '1' ]; then
	echo $USB_NUM | sudo tee /sys/bus/usb/drivers/usb/bind &>/dev/null || echo "Already on"
elif [ $(eval echo \${$#}) == '0' ]; then
	echo $USB_NUM | sudo tee /sys/bus/usb/drivers/usb/unbind &>/dev/null|| echo "Already off."
else
	print_help
fi
