
![24-touch-midi-controller](https://github.com/danonwski/24-touch-midi-controller/blob/main/media/24-touch-midi-controller.jpg)


# 24 Touch MIDI Controller (Atmega32u4) 
The controller has 24 capacitive inputs supported by two MPR121 chips. The MCU is Arduin Pro Micro with Atmega32u4 supporting MIDI communication via USB.

## Parts:
- 1x Arduino Pro Micro (atmega32u4)

- 2x MPR121 breakout

- 2x LED Ws2812B

- 2x  button

- 1x AMS1117 +  2x 100nf capacitor (3.3 stabilizer voltage)

### MPR121
 
Mpr121 run only on +3.3 voltage, you must use voltage regulator for example AMS1117.
- change default adress (0x5A) - cut jumper ADD/GND , and connct ADD to 3v3 (0x5B), ADD to SDA (0x5C), ADD to SCL (0x5D)

- you can connect 4 MPR121 at once i2C

## Connections:

- MPR121 → I2c - sda pin - 2 / scl pin - 3  (+ sda and scl 4,7k  to 3.3vcc) without logic conver

- Down button → Pin 5

- Up button → Pin 4

- Led ws2812b → Pin  6



## Libary:

Code usage Adafruit MPR1121, Adafruit NeoPixel, MIDIUSB and avdweb_Switch 

## Function:

Mpr121 send MIDI value start from C1 - 36 note midi


- Single click DownButton - change octave -1

- Single click UpButton - change octave -1

- long press DownButton - mute all channels

- long press UpButton - resetting mpr threshold + unmute all channels


## Status:

- first led - generate rgb fade effect

- second led - show status connector touched, mute/unmute mode, current change octave





## To do in the future:

-more button & rotary encoder // control change ( play, rec, pause, stop, overdub)
