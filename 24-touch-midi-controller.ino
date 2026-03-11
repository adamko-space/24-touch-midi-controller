#include <Adafruit_MPR121.h>
#include <MIDIUSB.h>
#include <Adafruit_NeoPixel.h>
#include "Arduino.h"
#include "avdweb_Switch.h" 

#define PIN 6
#define NUM_LEDS 2
#define THRESHOLD 4
#define RELEASE 4
#define BASE_NOTE_MIN 24 //C-1
#define BASE_NOTE_MAX 94 //C8
#define BASE_NOTE 36 //def note C1
#define MIDI_VELOCITY 0x7F //127 velocity
#define BRIGHTNESS 30
#define DEBOUNCE_TIME 50 // debounce time in milliseconds
#define HOLD_CONFIRMATION_TIME 100 // czas potwierdzenia utrzymania dotyku (ms)
#define FADE_MIN_BRIGHTNESS 30 // minimalna jasność fade
#define FADE_MAX_BRIGHTNESS 150 // maksymalna jasność fade
#define FADE_SPEED 0.005 // szybkość fade (im mniejsze, tym wolniejsze)

const byte downButtonPin  = 5; // downButton 
const byte upButtonPin = 4; // upButton

Switch downButton = Switch(downButtonPin);
Switch upButton = Switch(upButtonPin);

bool isMuted = false; // midi muted

Adafruit_MPR121 cap1 = Adafruit_MPR121(); //initialize first mpr121
Adafruit_MPR121 cap2 = Adafruit_MPR121(); //initialize second mpr121
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800); //initialize ws8212b

// Rozszerzona struktura przechowywania stanu
bool last_touched[24] = {false}; // Stan każdego pada (true = dotknięty)
bool note_active[24] = {false}; // Czy nota MIDI jest aktywna
unsigned long last_touch_time[24] = {0}; // Czas ostatniej zmiany stanu
unsigned long touch_start_time[24] = {0}; // Czas rozpoczęcia dotyku
uint8_t touch_stability_counter[24] = {0}; // Licznik stabilności sygnału

// Zmienna do fade (używa wartości float dla płynności)
float fadePhase = 0.0;

int baseNote = BASE_NOTE; // set base note

void setup() {
  Serial.begin(9600);
  cap1.begin(0x5B); // initialization MPR121 adress 0x5B
  cap1.setThresholds(THRESHOLD, RELEASE); // set treshold MPR121 adress 0x5B
  cap2.begin(0x5A); //initialization MPR121 adress 0x5A
  cap2.setThresholds(THRESHOLD, RELEASE); // set treshold MPR121 adress 0x5A
  strip.begin(); //initialization LED
  strip.setBrightness(BRIGHTNESS); // set default brighthness
}

void loop() {
  readTouchInputs(&cap1, 0); // waiting for touch first mpr121
  readTouchInputs(&cap2, 12); // waiting for touch second mpr121
  
  // Płynny fade LED 0 z kolorem oktawy
  fadeOctaveDisplay();
  
  strip.show();
  
  downButton.poll(); 

  if(downButton.singleClick()) {
    if (baseNote > BASE_NOTE_MIN) {
      baseNote -= 12; 
      Serial.print("Oktawa: ");
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
    if (baseNote < BASE_NOTE_MAX) {
      baseNote += 12; 
      Serial.print("Oktawa: ");
      Serial.println(baseNote);
      blinkLed(1, getColor(baseNote)); 
      if (isMuted) { 
        strip.setPixelColor(1, strip.Color(255, 0, 0)); 
        strip.show();
      }
    }
  }

  if(upButton.longPress() && isMuted) {
    strip.setPixelColor(1, strip.Color(0, 255, 0)); 
    strip.show();
    delay(400);
    cap1.setThresholds(THRESHOLD, RELEASE);
    cap2.setThresholds(THRESHOLD, RELEASE);
    unmuteAllChannels(); 
  }
}

// Funkcja płynnego fade LED 0 z kolorem oktawy (breathing effect)
void fadeOctaveDisplay() {
  // Pobierz bazowy kolor oktawy
  uint32_t baseColor = getColor(baseNote);
  
  // Wyodrębnij składowe RGB
  uint8_t r = (baseColor >> 16) & 0xFF;
  uint8_t g = (baseColor >> 8) & 0xFF;
  uint8_t b = baseColor & 0xFF;
  
  // Oblicz jasność używając sinusa dla płynnego fade
  // sin zwraca wartości od -1 do 1, przekształcamy na zakres 0-1
  float sineWave = (sin(fadePhase) + 1.0) / 2.0;
  
  // Mapuj sinusoidę na zakres jasności
  int currentBrightness = FADE_MIN_BRIGHTNESS + (int)(sineWave * (FADE_MAX_BRIGHTNESS - FADE_MIN_BRIGHTNESS));
  
  // Skaluj jasność
  float brightnessFactor = (float)currentBrightness / 255.0;
  uint8_t scaledR = (uint8_t)(r * brightnessFactor);
  uint8_t scaledG = (uint8_t)(g * brightnessFactor);
  uint8_t scaledB = (uint8_t)(b * brightnessFactor);
  
  // Ustaw kolor z przeskalowaną jasnością
  strip.setPixelColor(0, strip.Color(scaledR, scaledG, scaledB));
  
  // Zwiększ fazę dla następnej klatki (tworzy ciągły ruch)
  fadePhase += FADE_SPEED;
  
  // Zapobiegnij przepełnieniu float
  if (fadePhase > TWO_PI * 100) {
    fadePhase -= TWO_PI * 100;
  }
}

void readTouchInputs(Adafruit_MPR121* cap, int offset) {
  uint16_t current_touched = cap->touched();
  unsigned long currentTime = millis();
  
  for (uint8_t i = 0; i < 12; i++) {
    int padIndex = i + offset;
    bool isCurrentlyTouched = (current_touched & _BV(i)) != 0;
    
    // === LOGIKA Z HISTEREZĄ ===
    
    if (isCurrentlyTouched) {
      // Pad jest dotknięty w tym cyklu
      
      if (!last_touched[padIndex]) {
        // Nowy dotyk - rozpocznij proces
        last_touched[padIndex] = true;
        touch_start_time[padIndex] = currentTime;
        touch_stability_counter[padIndex] = 1;
      } else {
        // Dotyk kontynuowany - zwiększ licznik stabilności
        if (currentTime - last_touch_time[padIndex] >= 10) { // Co 10ms
          touch_stability_counter[padIndex]++;
          last_touch_time[padIndex] = currentTime;
        }
      }
      
      // Aktywuj notę dopiero gdy dotyk jest stabilny przez określony czas
      if (!note_active[padIndex] && 
          touch_stability_counter[padIndex] >= 3) { // 3 kolejne odczyty = ~30ms stabilności
        handleTouchOn(i, offset);
        note_active[padIndex] = true;
        Serial.print("Pad ");
        Serial.print(padIndex);
        Serial.println(" - nota aktywowana");
      }
      
    } else {
      // Pad NIE jest dotknięty w tym cyklu
      
      if (note_active[padIndex]) {
        // Nota jest aktywna - wymaga potwierdzenia puszczenia
        
        if (last_touched[padIndex]) {
          // Pierwsze wykrycie braku dotyku - rozpocznij liczenie
          last_touched[padIndex] = false;
          last_touch_time[padIndex] = currentTime;
          touch_stability_counter[padIndex] = 0;
        }
        
        // Deaktywuj notę dopiero po potwierdzonym czasie bez dotyku
        if (currentTime - last_touch_time[padIndex] >= HOLD_CONFIRMATION_TIME) {
          handleTouchOff(i, offset);
          note_active[padIndex] = false;
          touch_stability_counter[padIndex] = 0;
          Serial.print("Pad ");
          Serial.print(padIndex);
          Serial.println(" - nota dezaktywowana");
        }
      } else {
        // Nota nieaktywna i brak dotyku - reset stanów
        last_touched[padIndex] = false;
        touch_stability_counter[padIndex] = 0;
      }
    }
  }
}

void handleTouchOn(uint8_t i, int offset) {
  if (!isMuted) { 
    noteOn(0x00, baseNote + i + offset, MIDI_VELOCITY);
    MidiUSB.flush();
    Serial.print("→ NoteON: "); 
    Serial.println(baseNote + i + offset);
    strip.setPixelColor(1, strip.Color(255, 255, 246)); 
  }
}

void handleTouchOff(uint8_t i, int offset) {
  if (!isMuted) { 
    noteOff(0x00, baseNote + i + offset, 0);
    MidiUSB.flush();
    Serial.print("← NoteOFF: "); 
    Serial.println(baseNote + i + offset);
    strip.setPixelColor(1, strip.Color(0, 0, 0)); 
  }
}

// function noteOn
void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

// function noteOff
void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

// function CC change
void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t controlChange = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(controlChange);
}

// function mute all channels
void muteAllChannels() {
  // Najpierw wyślij noteOff dla wszystkich aktywnych nut
  for (uint8_t i = 0; i < 24; i++) {
    if (note_active[i]) {
      noteOff(0x00, baseNote + i, 0);
      note_active[i] = false;
    }
  }
  MidiUSB.flush();
  
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
  
  // Wyczyść wszystkie stany
  for (uint8_t i = 0; i < 24; i++) {
    last_touched[i] = false;
    note_active[i] = false;
    last_touch_time[i] = 0;
    touch_start_time[i] = 0;
    touch_stability_counter[i] = 0;
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