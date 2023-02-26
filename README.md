# SD_Swap

## A development tool which swaps an SD card between a USB3 card reader or a SBC.

### Software
To swap the SD card between the host PC and the SBC you run
`./swap.sh 0` or `./swap.sh 1`. 1 is the host PC while 0 is the SBC. You can also chose which device to use if you have multiple attached. To do this run `./swap.sh 1` to print the found devices and type `./swap.sh 1-4 1` or another usb number. I have future plans to write this script for Windows and OSX.

```
$ ./swap.sh -h

To switch the SD card to USB run:
./swap.sh 1
To switch the SD card to SBC run:
./swap.sh 0
If you have multiple devices run either of the above.
Then run with the last param the same as above:
./swap.sh $(Your USB number) 0

Examples:
# ./swap.sh 0
Uisng device /sys/bus/usb/devices/3-4

# ./swap.sh 0
Uisng device /sys/bus/usb/devices/3-4
Already off.

# ./swap.sh 0
Multiple devices found! Pass a usb number as param 1...
Usb num: 3-4
Usb num: 3-1

# ./swap.sh 3-1 0
Uisng device /sys/bus/usb/devices/3-1
```

To manually controll the port use:

```
export YOUR_USB_NUM="1-4" #hub1 port4
echo 0 | sudo tee /sys/bus/usb/devices/${YOUR_USB_NUM}/power/autosuspend_delay_ms
echo auto | sudo tee /sys/bus/usb/devices/${YOUR_USB_NUM}/power/control

echo SBC
echo ${YOUR_USB_NUM} | sudo tee /sys/bus/usb/drivers/usb/unbind

echo PC
echo ${YOUR_USB_NUM} | sudo tee /sys/bus/usb/drivers/usb/bind
```
### Hardware
The hardware is still WIP, but I have a mostly working prototype.
![Front image of PCB](https://github.com/Mr-Bossman/SD_Swap/blob/master/images/Front.jpg?raw=true)
![Back image of PCB](https://github.com/Mr-Bossman/SD_Swap/blob/master/images/Back.jpg?raw=true)
<!--
![Front image of rendered PCB](https://github.com/Mr-Bossman/SD_Swap/blob/master/images/Rendered_Front.jpg?raw=true)
![Back image of rendered PCB](https://github.com/Mr-Bossman/SD_Swap/blob/master/images/Rendered_Back.jpg?raw=true)
-->

### About
After seeing the [SDWire](https://wiki.tizen.org/SDWire) project I was disappointed by the complexity as well as the lack of USB3. So this project is uses the REALTEK RTS5306E which many USB3 to SD adapers implement, the chip has suspend functionality which we will use as a GPIO for switching hosts. The suspend option will shut off the SD cards power but not the controllers; it also has build-in RW protection as to finnish writes before switching to the SBC

The SDWire project uses a USB to GPIO chip for this, but they have 2 power controlled USB ports on the board as well as the above mentioned method.
