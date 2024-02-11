#include <Adafruit_MPR121.h>
#include <MIDIUSB.h>
#include <Adafruit_NeoPixel.h>
#include "Arduino.h"
#include "avdweb_Switch.h" 

#define PIN 6
#define NUM_LEDS 2
#define THRESHOLD 2
#define RELEASE 1
#define BASE_NOTE_MIN 24 //C-1
#define BASE_NOTE_MAX 94 //C8
#define BASE_NOTE 36 //def note C1
#define MIDI_VELOCITY 0x7F //127 velocity
#define BRIGHTNESS 30

const byte downButtonPin  = 5; // downButton 
const byte upButtonPin = 4; // upButton

Switch downButton = Switch(downButtonPin);
Switch upButton = Switch(upButtonPin);


bool isMuted = false; // midi muted

Adafruit_MPR121 cap1 = Adafruit_MPR121(); //initialize first mpr121
Adafruit_MPR121 cap2 = Adafruit_MPR121(); //initialize second mpr121
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800); //initialize ws8212b

uint16_t last_touched[24] = {0};

long pixelHue = 0;

int baseNote = BASE_NOTE; // set base note

void setup() {
  Serial.begin(9600);
  cap1.begin(0x5B); // initialization MPR121 adress 0x5B
  cap1.setThresholds(THRESHOLD, RELEASE); // set treshold MPR121 adress 0x5B
  cap2.begin(0x5A); //initialization MPR121 adress 0x5A
  cap2.setThresholds(THRESHOLD, RELEASE); // Uset treshold MPR121 adress 0x5A
  strip.begin(); //initialization LED
  strip.setBrightness(BRIGHTNESS); // set default brighthness
}

void loop() {
  readTouchInputs(&cap1, 0); // waiting for touch first mpr121
  readTouchInputs(&cap2, 12); // waiting for touch second mpr121
  RGBFade(0); // fade effect on led 1
  strip.show();
  
  downButton.poll(); 

  if(downButton.singleClick()) {
    if (baseNote >= BASE_NOTE_MIN) { 
      baseNote -= 12; 
      Serial.println(baseNote); 
      blinkLed(1, getColor(baseNote)); 
      if (isMuted) { 
        strip.setPixelColor(1, strip.Color(255, 0, 0)); 
        strip.show();
      }
    }
  }

  if(downButton.longPress()) {
    if (!isMuted) { 
      muteAllChannels(); 
      strip.setPixelColor(1, strip.Color(255, 0, 0));
      strip.show();
    }
  }

  upButton.poll(); 

  if(upButton.singleClick()) {
    if (baseNote <= BASE_NOTE_MAX) { 
      baseNote += 12; 
      Serial.println(baseNote);
      blinkLed(1, getColor(baseNote)); 
      if (isMuted) { 
        strip.setPixelColor(1, strip.Color(255, 0, 0)); 
        strip.show();
      }
    }
  }

  if(upButton.longPress() && isMuted) {
    if (isMuted) { 
      strip.setPixelColor(1, strip.Color(0, 255, 0)); 
      strip.show();
      delay(400);
      cap1.setThresholds(THRESHOLD, RELEASE);
      cap2.setThresholds(THRESHOLD, RELEASE);
      unmuteAllChannels(); 
      
      
    }
  }
}


void readTouchInputs(Adafruit_MPR121* cap, int offset) {
  uint16_t current_touched = cap->touched();
  for (uint8_t i=0; i<12; i++) {
    
    if ((current_touched & _BV(i)) && !(last_touched[i + offset] & _BV(i))) {
      handleTouchOn(i, offset);
    }
    
    else if (!(current_touched & _BV(i)) && (last_touched[i + offset] & _BV(i))) {
      handleTouchOff(i, offset);
    }
    last_touched[i + offset] = current_touched;
  }
}

void handleTouchOn(uint8_t i, int offset) {
  if (!isMuted) { 
    noteOn(0x90, baseNote + i + offset, MIDI_VELOCITY); 
    MidiUSB.flush();
    Serial.print("Wysłano noteOn z wartością "); 
    Serial.println(baseNote + i + offset);
    strip.setPixelColor(1, strip.Color(255, 255, 246)); 
  }
}

void handleTouchOff(uint8_t i, int offset) {
  if (!isMuted) { 
    noteOff(0x80, baseNote + i + offset, 0); 
    MidiUSB.flush();
    Serial.print("Wysłano noteOff z wartością "); 
    Serial.println(baseNote + i + offset);
    strip.setPixelColor(1, strip.Color(0, 0, 0)); 
  }
}

// rgb fade effect
void RGBFade(int pixel) {
  strip.setPixelColor(pixel, strip.gamma32(strip.ColorHSV(pixelHue)));
  pixelHue += 12;
  if(pixelHue > 3*65536) {
    pixelHue = 0;
  }
}

// function noteOn
void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

// function noteOff
void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

// function CC change
void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t controlChange = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(controlChange);
}

// function mute all channels
void muteAllChannels() {
  for (byte channel = 0; channel < 16; channel++) {
    controlChange(channel, 120, 0); 
  }
  isMuted = true;
}

// function unmute all channels
void unmuteAllChannels() {
  for (byte channel = 0; channel < 16; channel++) {
    controlChange(channel, 121, 0);
  }
  isMuted = false;
  
  for (uint8_t i=0; i<24; i++) {
    last_touched[i] = 0;
  }
}

// change flash color when change octave
uint32_t getColor(int baseNote) {
  switch (baseNote) {
    case 12: return strip.Color(252, 3, 3); // red 
    case 24: return strip.Color(252, 136, 3); // orange
    case 36: return strip.Color(252, 235, 3); // yellow
    case 48: return strip.Color(132, 252, 3); // green
    case 60: return strip.Color(3, 252, 219); // celeste
    case 72: return strip.Color(3, 94, 252); // blue
    case 84: return strip.Color(123, 3, 252); // violet
    case 96: return strip.Color(252, 3, 235); // pink
    default: return strip.Color(255, 255, 255); // white
  }
}

// confirmation octave - led blinks
void blinkLed(int pixel, uint32_t color) {
  for (int i = 0; i < 2; i++) {
    strip.setPixelColor(pixel, color);
    strip.show();
    delay(100);
    strip.setPixelColor(pixel, strip.Color(0, 0, 0));
    strip.show();
    delay(100);
  }
}