#!/bin/bash

set -e

cd $(dirname "$0")

echo "Swaping SD card to USB..."
sdswap 1
DISK=$(sdswap p)

echo "Flashing to $DISK..."

sudo dd if=buildroot/output/images/sdcard.img of=$DISK bs=4M oflag=sync status=progress
sudo sync
sdswap 0

echo "Resetting the board..."
# Use the DTR line to reset the board
./reset.py $1
echo "Board reset."
