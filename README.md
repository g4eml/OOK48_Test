# OOK48 Test Device

## Description

Test device for OOK48 Protocol. 

Message is user configurable using the USB Serial port. Just run any terminal program, enter CR and then follow the prompt. 

Key Output on Pin 6

800 Hz Keyed Tone on Pin 2

Random Noise on Pin 3

1PPS sync signal input or output on Pin 7.

GPS NMEA output and input on Pins 4 and 5.

Key Input on Pin 8 can be used by fitting link from pin 10 to ground. Key input then directly controls the Tone Output. Used for testing the Tx side of a device. 

Fit link from Pin 11 to ground to select 2 second mode.

Fit link from Pin 13 to ground when connecting a real GPS module. Unit can then be used as a stand alond beacon generator.
If no link is fitted to pin 13 then the module will simulate a GPS module and output the 1PPS pulse and fake NMEA 9600 Baud data. 









