# OOK48 Test Device

## Description

Test device for OOK48 Protocol. 

Constantly sends the test message 'OOK48 TEST'

Key Output on Pin 6

800 Hz Keyed Tone on Pin 2

Random Noise on Pin 3

1PPS sync signal output on Pin 7. Simulates the GPS 1PPS signal. 

Key Input on Pin 8 can be used by fitting link to pin 10. Key input then directly controls the Tone Output. Used for testing the Tx side of a device. 

Fit link from Pin 11 to select 2 second mode.  Note this has a 50% chance of working as it needs to be in sync with the receiver. You may need to remove and refit the link a couple of times until it works. 





