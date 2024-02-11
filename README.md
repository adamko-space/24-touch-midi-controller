The controller has 24 capacitive inputs supported by two MPR121 chips. The MCU is Arduin Pro Micro with Atmega32u4 supporting MIDI communication via USB.


Mpr121 run only on +3.3 voltage, you must use voltage regulator for example AMS1117.

Connections:

-MPR121 → I2c - sda pin - 2 / scl pin - 3  (+ sda and scl 4,7k  to 3.3vcc) without logic conver

-Down button → Pin 5

-Up button → Pin 4

-Led ws2812b → Pin  6



Libary:

Code usage Adafruit MPR1121, Adafruit NeoPixel, MIDIUSB and avdweb_Switch 

Function:

Mpr121 send MIDI value start from C1 - 36 note midi


-Single click DownButton - change octave -1

-Single click UpButton - change octave -1

-long press DownButton - mute all channels

-long press UpButton - resetting mpr threshold + unmute all channels


Status:

-first led - generate rgb fade effect

-second led - show status connector touched, mute/unmute mode, current change octave





To do in the future:

-more button & rotary encoder // control change ( play, rec, pause, stop, overdub)
