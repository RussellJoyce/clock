#include <FastSPI_LED2.h>
#include <LwRx.h>
#include <EEPROM.h>
#include "clock.h"

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
  initLEDs(maxBrightness);

  // Run intro sequence
  intro();

  // Start clock
  go = true;
  //showTime = true;
}


ISR(TIMER2_OVF_vect) {
  
  if (!go)
    return;

  if (!timeSet) {
    if (!animating)
      animation(ANIM_PULSER);
    return;
  }


  if (showTime) {
    leds[ledMap[secs]] = CRGB::Black;
    leds[ledMap[mins]] = CRGB::Black;
    leds[ledMap[((hrs % 12) * 5) + (mins / 12)]] = CRGB::Black;
  }

  incSecs();

  if (showTime) {
    leds[ledMap[secs]] += CRGB::Blue;
    leds[ledMap[mins]] += CRGB::Green;
    leds[ledMap[((hrs % 12) * 5) + (mins / 12)]] += CRGB::Red;
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
          leds[ledMap[((hrs % 12) * 5) + (mins / 12)]] = CRGB::Black;
        }

        if (type == ON)
          incSecs();
        else
          decSecs();

        if (showTime) {
          leds[ledMap[secs]] += CRGB::Blue;
          leds[ledMap[mins]] += CRGB::Green;
          leds[ledMap[((hrs % 12) * 5) + (mins / 12)]] += CRGB::Red;
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
  animating = true;
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
    for (int i = 0; i < 26; i++) {
      LEDS.setBrightness(i);
      LEDS.show();
      delay(6);
    }
    for (int i = 0; i < 26; i++) {
      LEDS.setBrightness(25-i);
      LEDS.show();
      delay(6);
    }
  }

  // Restore state
  //memcpy(leds, ledsBackup, NUM_LEDS * sizeof(CRGB));
  LEDS.setBrightness(brightness);
  LEDS.clear();
  showTime = showedTime;
  animating = false;
}

