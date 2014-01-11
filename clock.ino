#include "FastSPI_LED2.h"
#include <LwRx.h>
#include <EEPROM.h>

#define DEBUG 1

#define NUM_LEDS 60
#define DATA_PIN 11


/**
 * RF receiver constants
 * Mood:      8 2 F 2
 * Hold mood: 0 2 F 2
 * All off:   C 0 F 0
 */

// RF button type - byte 3
#define OFF      0x0
#define ON       0x1
#define HOLDMOOD 0x2

// RF press type - byte 0
#define DOWN    0x0
#define UP      0x4
#define HOLDOFF 0xA
#define HOLDON  0xB
#define ALLOFF  0xC
#define MOOD    0x8

uint8_t remoteId[6];


// Animation types
#define ANIM_RAINBOW 0 // Rotating rainbow, fades in and out - lasts ~5.2s
#define ANIM_SPIN    1 // Two spinning single LEDs in opposite directions, one half speed - lasts ~3s
#define ANIM_PULSER  2 // Pulse all LEDs in red - lasts ~0.5s


// Standard LED map
uint8_t ledMap[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59};
// Backwards LED map
//uint8_t ledMap[] = {59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};


CRGB leds[NUM_LEDS];
//CRGB ledsBackup[NUM_LEDS];
int secs = 0;
int mins = 0;
int hrs = 0;

boolean go = false;
boolean showTime = false;
boolean timeSet = false;

byte msg[10];
byte len = 10;


void setup() {
  // initialize serial debug
#if (DEBUG)
  Serial.begin(9600);
#endif

  // Load remote ID from EEPROM
  loadRemote();


  // Set up 1 second timer interrupt
  cli(); // disable interrupts
  ASSR = 0x20; //enable timer2 async mode with an external crystal
  delay(250); //let external 32KHz crystal stabilize
  TCCR2A = 0x0;
  TCCR2B = 0x05; //set the prescaler to 0x04 (64): 32.768kHz / 128 = 1Hz overflow
  TIFR2 = 0x1; //reset timer2 overflow interrupt flag
  TIMSK2 = 0x01; //enable interrupt on overflow
  SMCR = 0x03; //use power-save mode when we sleep
  sei(); //enable all interrupts

  
  // set up LWRX with rx into pin 2
  lwrx_setup(2);

  // Set up LEDs
  LEDS.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  initLEDs(32);

  // Run intro sequence
  intro();

  // Start clock
  go = true;
  //showTime = true;
}


ISR(TIMER2_OVF_vect){
  
  if (!go)
    return;

  if (!timeSet) {
    animation(ANIM_PULSER);
    return;
  }


  if (showTime) {
    leds[ledMap[secs]] = CRGB::Black;
    leds[ledMap[mins]] = CRGB::Black;
    leds[ledMap[(hrs % 12) * 5]] = CRGB::Black;
  }

  incSecs();

  if (showTime) {
    leds[ledMap[secs]] += CRGB::Blue;
    leds[ledMap[mins]] += CRGB::Green;
    leds[ledMap[(hrs % 12) * 5]] += CRGB::Red;
    LEDS.show();
  }
  
#if (DEGUG)
  // Print time
  if (hrs < 10) Serial.print("0");
  Serial.print(hrs);
  if (mins < 10)
    Serial.print(":0");
  else
    Serial.print(":");
  Serial.print(mins);
  if (secs < 10)
    Serial.print(":0");
  else
    Serial.print(":");
  Serial.print(secs);
  Serial.print("\n");
#endif
}


void loop() {
  if (lwrx_message()) {
    lwrx_getmessage(msg,&len);
#if (DEBUG)
    printMsg(msg, len);
#endif

    // Check for holding down mood - register remote
    if (msg[0] == 0x0 &&
        msg[1] == 0x2 &&
        msg[2] == 0xF &&
        msg[3] == 0x2) {

#if (DEBUG)
      Serial.print("Registering new remote...\n");
#endif 

      remoteId[0] = msg[4];
      remoteId[1] = msg[5];
      remoteId[2] = msg[6];
      remoteId[3] = msg[7];
      remoteId[4] = msg[8];
      remoteId[5] = msg[9];

      // Save remote address to EEPROM
      saveRemote();

      // Animate, partly to act as a delay
      animation(ANIM_SPIN);
    }

    // Check remote ID matches
    else if (remoteId[0] == msg[4] &&
             remoteId[1] == msg[5] &&
             remoteId[2] == msg[6] &&
             remoteId[3] == msg[7] &&
             remoteId[4] == msg[8] &&
             remoteId[5] == msg[9]) {

      int btn = msg[2];
      int type = msg[3];

      if (btn == 0) {
        go = (type == ON);
      }
      else if (btn == 1) {

        if (showTime) {
          leds[ledMap[secs]] = CRGB::Black;
          leds[ledMap[mins]] = CRGB::Black;
          leds[ledMap[(hrs % 12) * 5]] = CRGB::Black;
        }

        if (type == ON)
          incSecs();
        else
          decSecs();

        if (showTime) {
          leds[ledMap[secs]] += CRGB::Blue;
          leds[ledMap[mins]] += CRGB::Green;
          leds[ledMap[(hrs % 12) * 5]] += CRGB::Red;
          LEDS.show();
        }

        if (!timeSet) {
          timeSet = true;
          showTime = true;
        }

      }
      else if (btn == 2) {
        if (type == ON)
          animation(ANIM_SPIN);
        else if (type == OFF)
          animation(ANIM_RAINBOW);
      }
    }
  }
}

void initLEDs(byte brightness) {
  LEDS.setBrightness(brightness);
  LEDS.clear();
}

void intro() {
  animation(ANIM_RAINBOW);
}

void incSecs() {
  secs++;
  if (secs == 60) {
    secs = 0;
    mins++;
  }
  if (mins == 60) {
    mins = 0;
    hrs++;
  }
  if (hrs == 24) {
    hrs = 0;
  }
}

void decSecs() {
  secs--;
  if (secs == -1) {
    secs = 59;
    mins--;
  }
  if (mins == -1) {
    mins = 59;
    hrs--;
  }
  if (hrs == -1) {
    hrs = 23;
  }
}

void saveRemote() {
  uint8_t addr;

  addr = remoteId[0] << 4;
  addr |= remoteId[1] & 0xF;
  EEPROM.write(0, addr);
  addr = remoteId[2] << 4;
  addr |= remoteId[3] & 0xF;
  EEPROM.write(1, addr);
  addr = remoteId[4] << 4;
  addr |= remoteId[5] & 0xF;
  EEPROM.write(2, addr);

#if (DEBUG)
  Serial.print("Saved remote ID: ");
  for(int i = 0; i < 6; i++) {
    Serial.print(remoteId[i], HEX);
  }
  Serial.println();
#endif
}

void loadRemote() {
  uint8_t addr;

  addr = EEPROM.read(0);
  remoteId[0] = addr >> 4;
  remoteId[1] = addr & 0xF;
  addr = EEPROM.read(1);
  remoteId[2] = addr >> 4;
  remoteId[3] = addr & 0xF;
  addr = EEPROM.read(2);
  remoteId[4] = addr >> 4;
  remoteId[5] = addr & 0xF;

#if (DEBUG)
  Serial.print("Loaded remote ID: ");
  for(int i = 0; i < 6; i++) {
    Serial.print(remoteId[i], HEX);
  }
  Serial.println();
#endif
}

void printMsg(byte *msg, byte len) {
  Serial.print(millis());
  Serial.print(" ");
  for(int i=0;i<len;i++) {
    Serial.print(msg[i],HEX);
    Serial.print(" ");
  }
  Serial.println();
}

void animation(int type) {
  // Save state
  boolean showedTime = showTime;
  uint8_t brightness = LEDS.getBrightness();
  //memcpy(ledsBackup, leds, NUM_LEDS * sizeof(CRGB));

  // Clear LEDs and stop time display
  showTime = false;
  LEDS.clear();

  if (type == ANIM_RAINBOW) {
    int brightnessScale = 256/brightness;
    LEDS.setBrightness(0);
  
    for (int i = 0; i < 255; i++) {
      fill_rainbow(leds, NUM_LEDS, i % 255, 255/NUM_LEDS);
      LEDS.show();
      delay(1);
      LEDS.setBrightness(i/brightnessScale);
    }

    LEDS.setBrightness(brightness);

    for (int i = 0; i < ((255 * 2) - 1); i++) {
      fill_rainbow(leds, NUM_LEDS, i % 255, 255/NUM_LEDS);
      LEDS.show();
      delay(2);
    }

    for (int i = 0; i < 255; i++) {
      fill_rainbow(leds, NUM_LEDS, i % 255, 255/NUM_LEDS);
      LEDS.show();
      delay(1);
      LEDS.setBrightness(brightness-(i/brightnessScale));
    }
  }
  else if (type == ANIM_SPIN) {
    for (int i = 0; i < NUM_LEDS * 4; i++) {
      leds[ledMap[i % NUM_LEDS]] = CRGB::White;
      leds[ledMap[(NUM_LEDS - 1) - ((i/2) % NUM_LEDS)]] = CRGB::White;
      LEDS.show();
      delay(10);
      leds[ledMap[i % NUM_LEDS]] = CRGB::Black;
      leds[ledMap[(NUM_LEDS - 1) - ((i/2) % NUM_LEDS)]] = CRGB::Black;
    }
  }
  else if (type == ANIM_PULSER) {
    LEDS.setBrightness(0);
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB::Red;
    }
    for (int i = 0; i < 16; i++) {
      LEDS.setBrightness(i);
      LEDS.show();
      delay(12);
    }
    for (int i = 0; i < 16; i++) {
      LEDS.setBrightness(15-i);
      LEDS.show();
      delay(12);
    }
  }

  // Restore state
  //memcpy(leds, ledsBackup, NUM_LEDS * sizeof(CRGB));
  LEDS.setBrightness(brightness);
  LEDS.clear();
  showTime = showedTime;
}

