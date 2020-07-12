# FridgeFan_1
Monitor and adjust fan speed, display on 12 seg RYG led bar graph using 74HC595 shift registers

 Display fridge temp on 12 seg led bar graph with 2 * 74HC595 shift registers
 NB the led bar graph when all on draws ~0.07A.
 Led and shift registers therfore require separate 5V supply as Arduino via USB not enough at 0.025A
 
 Hardware:
 * 12 segment led KWL-R1230CDUGB 14 pin.Common cathode 
 * 2 led's /segment Red + Green = 24 total + both on = yellow
 * 2 * 74HC595 shift register daisy chained.
 * Register 1 connected to anodes (8) pins (R/G) 2/13;3/12;4/11;5/10 via 330 ohm R
 * Register 2 Q0, Q1 & Q2 connected to base of npn transistors as switches
 * Led cathodes pins 1/14,6/9,7/8 connected to collector of npn transistor via 10K ohm R
 * LEDs attached to each of the outputs of the shift register (8) * 3 = 24
 * Only 3 outputs are needed for cathode register so only MSB bytes 1-7 required
 * Cathode byte 1 = 100; segments A-D
 * Cathode byte 2 = 010; segments E-H
 * Cathode byte 3 = 110; segments A-H
 * Cathode byte 4 = 001; segments I-L
 * Cathode byte 5 = 101; segments A-D & I-L
 * Cathode byte 6 = 011; segments E-L
 * Cathode byte 7 }= 111; segments A-L
 * ie for all on "Yellow" = Cathode byte 7 & Anode Byte 255 "111 & 11111111"
 * Cathode & Anode data stored in arrays
 
NB Fans run at 12V don't mix up!!!

Get lux levels so led's can be turned down at night
Deleted weathershield data so this now only recording Fridge temp
