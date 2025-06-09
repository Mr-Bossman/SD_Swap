#!/bin/bash

set -e

cd "$(dirname "$0")"

echo "Swaping SD card to USB..."
DISK=$(sdswap -pc)

echo "Flashing to $DISK..."

sudo dd if=buildroot/output/images/sdcard.img of="$DISK" bs=4M oflag=sync status=progress
sudo sync
sdswap -t

echo "Resetting the board..."
# Use the DTR line to reset the board
./reset.py "$1"
echo "Board reset."
