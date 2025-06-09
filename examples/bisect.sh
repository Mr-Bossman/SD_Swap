#!/bin/bash

passed() {
	local CNT
	stty -F "$1" 115200
	CNT=$(grep -m 2 'U-Boot' "$1"| grep -c 'U-Boot SPL')
	if [ "$CNT" -eq 1 ]; then
		echo "PASSED"
		return 0
	else
		echo "FAILED"
		return 1
	fi
}

flash() {
	sudo dd if=u-boot-with-spl.sfp of="$1" oflag=sync status=progress; sync
}

runmake () {
	make mrproper
	make CROSS_COMPILE=arm-none-eabi- ARCH=arm -j4 socfpga_de1_soc_defconfig
	make CROSS_COMPILE=arm-none-eabi- ARCH=arm -j4
}

test() {
	runmake
	echo "Swaping SD card to USB..."
	DEV=$(sdswap -pc)
	echo "Flashing to $DISK..."
	flash "$DEV"
	sdswap -t
	echo Please reset board
	passed "$1"
}

#git bisect start
#git bisect bad v2024.10-rc6
#git bisect good v2024.07-rc1
#git bisect run ./bisect.sh /dev/ttyUSB0
test "$1"