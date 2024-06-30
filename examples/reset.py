#!/usr/bin/env python3
from time import sleep
import sys
from serial import Serial


if len(sys.argv) < 2:
	print("Usage: " + sys.argv[0] + " <port>")
	sys.exit(1)

#ser = Serial(str(sys.argv[1]), int(sys.argv[2]))
ser = Serial(str(sys.argv[1]), 9600)

# Use the DTR line to reset the board
ser.setDTR(True)
sleep(0.1)
ser.setDTR(False)
ser.close()
