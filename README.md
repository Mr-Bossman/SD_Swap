# Auto SD swapper

After seeing the [SDWire](https://wiki.tizen.org/SDWire) project I was dissapointed by the compleity aswell as the lack of usb3.

This project is going to use the REALTEK RTS5306E.
Many USB to SD adapers implement a suspend functionality the RTS5306 also does
by runing 
`echo ${YOUR_USB_NUM} | sudo tee /sys/bus/usb/drivers/usb/unbind` it will shut off the sdcards power but not the controllers.
This is can be used as a GPIO to switch hosts, aswell as it has build-in RW protection as to finnish writes before swithing to the SBC

The SDWire project uses a USB to GPIO chip for this but they have 2 power controlled USB ports on the board aswell as the above mentioned methoud.

