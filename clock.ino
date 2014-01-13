#include <FastSPI_LED2.h>
#include <LwRx.h>
#include <EEPROM.h>
#include "clock.h"


void setup() {
  // initialize serial debug
#if (DEBUG)
  Serial.begin(19200);
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
    if (!animating && !turnedOff && !settingChanged)
      animation(ANIM_PULSER);
    else if (settingChanged)
      settingChanged = false;
    return;
  }

  if (timeChanged) {
    timeChanged = false;
    return;
  }


  if (!animating || animatingOff)
    memset(leds, 0, sizeof(leds));

  incTime();

  if (!animating || animatingOff) {

    // Add markers
    if (markerType == MRKR_EVERY5) {
      for (int i = 0; i < NUM_LEDS; i+=5) {
        leds[ledMap[i]] = LED_5;
      }
      leds[ledMap[0]]  = LED_60;
      leds[ledMap[15]] = LED_15;
      leds[ledMap[30]] = LED_15;
      leds[ledMap[45]] = LED_15;
    }
    else if (markerType == MRKR_EVERY15) {
      leds[ledMap[0]]  = LED_60;
      leds[ledMap[15]] = LED_15;
      leds[ledMap[30]] = LED_15;
      leds[ledMap[45]] = LED_15;
    }
    else if (markerType == MRKR_EVERY30) {
      leds[ledMap[0]]  = LED_60;
      leds[ledMap[30]] = LED_15;
    }
    else if (markerType == MRKR_60ONLY) {
      leds[ledMap[0]]  = LED_60;
    }

    clockTransform();
    tickAnimation();

    if (settingChanged)
      settingChanged = false;
  }
  
#if (DEBUG)
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

  // Check for outstanding LightwaveRF message
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

    // Check for 'all off' - toggle power
    else if (msg[0] == 0xC &&
             msg[1] == 0x0 &&
             msg[2] == 0xF &&
             msg[3] == 0x0) {

      // Do nothing if key repeat
      if ((millis() - lastOffButtonTime) > OFFDELAY) {
        if (turnedOff)
          turnOn();
        else
          turnOff();
      }

      lastOffButtonTime = millis();
    }

    // Check remote ID matches and cull repeats
    else if (!ignoreKey()) {

      int btn = msg[2];
      int type = msg[3];
      int pressType = msg[0];

      // Set hand brightness
      if (btn == BTN_HBRIGHT && pressType == DOWN) {

      }

      // Set hand colour
      else if (btn == BTN_HCOLOUR && pressType == DOWN) {

      }

      // Set tick style
      else if (btn == BTN_TICK && pressType == DOWN) {
        // On first press, just show current value, otherwise increment/decrement
        if (lastButton[2] == BTN_TICK && ((millis() - lastButtonTime) < 2000)) {
          if (type == OFF && tickStyle > 0) {
            tickStyle--;
          }
          else if (type == ON && tickStyle < (NUM_TICK - 1)) {
            tickStyle++;
          }
        }
        displaySettingValue(tickStyle, currentBrightness);
      }

      // Set clock style
      else if (btn == BTN_CLK && pressType == DOWN) {
        // On first press, just show current value, otherwise increment/decrement
        if (lastButton[2] == BTN_CLK && ((millis() - lastButtonTime) < 2000)) {
          if (type == OFF && clockStyle > 0) {
            clockStyle--;
          }
          else if (type == ON && clockStyle < (NUM_CLK - 1)) {
            clockStyle++;
          }
        }
        displaySettingValue(clockStyle, currentBrightness);
      }

      // Set marker brightness
      else if (btn == BTN_MBRIGHT && pressType == DOWN) {

      }

      // Set marker colour
      else if (btn == BTN_MCOLOUR && pressType == DOWN) {

      }

      // Set marker type
      else if (btn == BTN_MTYPE && pressType == DOWN) {
        // On first press, just show current value, otherwise increment/decrement
        if (lastButton[2] == BTN_MTYPE && ((millis() - lastButtonTime) < 2000)) {
          if (type == OFF && markerType > 0) {
            markerType--;
          }
          else if (type == ON && markerType < (NUM_MRKR - 1)) {
            markerType++;
          }
        }
        displaySettingValue(markerType, currentBrightness);
      }

      // Rotate clock
      else if (btn == BTN_ROTATE && pressType == DOWN) {

      }

      // Set power type
      else if (btn == BTN_POWER && pressType == DOWN) {
        // On first press, just show current value, otherwise increment/decrement
        if (lastButton[2] == BTN_POWER && ((millis() - lastButtonTime) < 2000)) {
          if (type == OFF && power > 0) {
            power--;
          }
          else if (type == ON && power < 60) {
            power++;
          }
        }
        displaySettingValue(power, 255);
      }

      // Correct time
      else if (btn == BTN_CORRECT && pressType == DOWN) {

      }

      // Strobing
      else if (btn == BTN_STROBE && pressType == DOWN) {
        animation(ANIM_STROBE);
      }

      // Pretty rainbow
      else if (btn == BTN_RAINBOW && pressType == DOWN) {
        animation(ANIM_RAINBOW);
      }

      // Set seconds/minutes/hours
      else if ((btn == BTN_HRS || btn == BTN_MINS || btn == BTN_SECS) && pressType != UP) {
        boolean showedTime = showTime;
        boolean going = go;
        showTime = false;
        go = false;

        // Clear LEDs array
        memset(leds, 0, sizeof(leds));

        // Add markers every 5 ticks
        for (int i = 5; i < NUM_LEDS; i+=5) {
          leds[ledMap[i]] = LED_SET5;
        }
        leds[ledMap[0]]  = LED_SET60;
        leds[ledMap[15]] = LED_SET15;
        leds[ledMap[30]] = LED_SET15;
        leds[ledMap[45]] = LED_SET15;

        
        // Increment/decrement value and display on clock
        if (type == ON) {
          if (btn == BTN_SECS) {
            secs++;
            if (secs == 60)
              secs = 0;
            leds[ledMap[secs]] = CRGB::White;
          }
          else if (btn == BTN_MINS) {
            mins++;
            if (mins == 60)
              mins = 0;
            leds[ledMap[mins]] = CRGB::White;
          }
          else if (btn == BTN_HRS) {
            hrs++;
            if (hrs == 12)
              hrs = 0;
            else if (hrs == 24)
              hrs = 12;
            leds[ledMap[(hrs % 12) * 5]] = CRGB::White;
          }
        }
        else if (type == OFF) {
          if (btn == BTN_SECS) {
            secs--;
            if (secs == -1)
              secs = 59;
            leds[ledMap[secs]] = CRGB::White;
          }
          else if (btn == BTN_MINS) {
            mins--;
            if (mins == -1)
              mins = 59;
            leds[ledMap[mins]] = CRGB::White;
          }
          else if (btn == BTN_HRS) {
            hrs--;
            if (hrs == -1)
              hrs = 11;
            else if (hrs == 11)
              hrs = 23;
            leds[ledMap[(hrs % 12) * 5]] = CRGB::White;
          }
        }


        showLeds();
        
        if (!timeSet) {
          timeSet = true;
          showTime = true;
        }
        else {
          showTime = showedTime;
        }
        timeChanged = true;
        go = going;
      }

      // Set AM/PM
      else if (btn == BTN_AMPM && pressType == DOWN) {
        if (type == OFF) {
          // Set to AM
          if (hrs > 11)
            hrs = hrs - 12;
          animation(ANIM_AM);
        }
        else if (type == ON) {
          // Set to PM
          if (hrs < 12)
            hrs = hrs + 12;
          animation(ANIM_PM);
        }

        if (!timeSet) {
          timeSet = true;
          showTime = true;
        }
      }

      // Store last button registered
      lastButton[0] = msg[0];
      lastButton[1] = msg[1];
      lastButton[2] = msg[2];
      lastButton[3] = msg[3];
      lastButtonTime = millis();
    }
  }
}


boolean ignoreKey() {
  // Ignore if turned off, wrong remote, or not a hold and equal to last button press within repeat delay
  return (turnedOff ||
          (remoteId[0] != msg[4] ||
           remoteId[1] != msg[5] ||
           remoteId[2] != msg[6] ||
           remoteId[3] != msg[7] ||
           remoteId[4] != msg[8] ||
           remoteId[5] != msg[9]) ||
          (msg[0] != HOLDON &&
           msg[0] != HOLDOFF &&
           lastButton[0] == msg[0] &&
           lastButton[1] == msg[1] &&
           lastButton[2] == msg[2] &&
           lastButton[3] == msg[3] &&
           (millis() - lastButtonTime) < KEYDELAY));
}


void initLEDs(byte brightness) {
  LEDS.setBrightness(brightness);
  LEDS.clear();
}


void intro() {
#if (!DEBUG)
  animation(ANIM_RAINBOW);
#endif
}


void incTime() {
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
      for (int j = 0; j < NUM_LEDS; j++) {
        leds[ledMap[j]].setHue((i % 255) + (j * (255 / NUM_LEDS)));
      }
      showLeds();
      LEDS.setBrightness(i/brightnessScale);
    }

    LEDS.setBrightness(brightness);

    for (int i = 0; i < ((255 * 2) - 1); i++) {
      for (int j = 0; j < NUM_LEDS; j++) {
        leds[ledMap[j]].setHue((i % 255) + (j * (255 / NUM_LEDS)));
      }
      showLeds();
    }

    for (int i = 0; i < 255; i++) {
      for (int j = 0; j < NUM_LEDS; j++) {
        leds[ledMap[j]].setHue((i % 255) + (j * (255 / NUM_LEDS)));
      }
      showLeds();
      LEDS.setBrightness(brightness-(i/brightnessScale));
    }
  }
  else if (type == ANIM_SPIN) {
    LEDS.setBrightness(maxBrightness);
    for (int i = 0; i < NUM_LEDS * 6; i++) {
      leds[ledMap[i % NUM_LEDS]] = CRGB::White;
      leds[ledMap[(NUM_LEDS - 1) - ((i/2) % NUM_LEDS)]] = CRGB::White;
      showLeds();
      delay(10);
      leds[ledMap[i % NUM_LEDS]] = CRGB::Black;
      leds[ledMap[(NUM_LEDS - 1) - ((i/2) % NUM_LEDS)]] = CRGB::Black;
    }
  }
  else if (type == ANIM_PULSER) {
    LEDS.setBrightness(0);
    for (int i = 0; i < 32; i++) {
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Red;
      }
      LEDS.setBrightness(i);
      showLeds();
      delay(4);
    }
    for (int i = 0; i < 32; i++) {
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Red;
      }
      LEDS.setBrightness(31-i);
      showLeds();
      delay(4);
    }
  }
  else if (type == ANIM_AM) {
    LEDS.setBrightness(0);
    for (int i = 15; i < 46; i++) {
      leds[ledMap[i]] = CRGB::White;
    }
    for (int i = 0; i < 21; i++) {
      LEDS.setBrightness(i);
      showLeds();
      delay(10);
    }
    for (int i = 0; i < 21; i++) {
      LEDS.setBrightness(20-i);
      showLeds();
      delay(10);
    }
  }
  else if (type == ANIM_PM) {
    LEDS.setBrightness(0);
    for (int i = 0; i < 16; i++) {
      leds[ledMap[i]] = CRGB::White;
    }
    for (int i = 45; i < 60; i++) {
      leds[ledMap[i]] = CRGB::White;
    }
    for (int i = 0; i < 21; i++) {
      LEDS.setBrightness(i);
      showLeds();
      delay(10);
    }
    for (int i = 0; i < 21; i++) {
      LEDS.setBrightness(20-i);
      showLeds();
      delay(10);
    }
  }

  else if (type == ANIM_STROBE) {
    memset(leds, 0xFF, sizeof(leds));
    for (int i = 0; i < 30; i++) {
      showLeds();
      delay(20);
      LEDS.showColor(CRGB::Black);
      delay(80);
    }

    randomSeed(micros());
    for (int i = 0; i < 720; i++) {
      leds[random(NUM_LEDS)].setHue(random(256));
      showLeds();
    }

    for (int i = 0; i < 30; i++) {
      showLeds();
      delay(20);
      LEDS.showColor(CRGB::Black);
      delay(80);
    }

    delay(3000);
  }

  // Restore state
  //memcpy(leds, ledsBackup, NUM_LEDS * sizeof(CRGB));
  LEDS.setBrightness(brightness);
  LEDS.clear();
  showTime = showedTime;
  animating = false;
}

void turnOff() {
  animating = true;
  animatingOff = true;
  currentBrightness = LEDS.getBrightness();
  int dly = 2048 / currentBrightness;
  
  for (int i = currentBrightness; i >= 0; i--) {
    LEDS.setBrightness(i);
    showLeds();
    delay(dly);
  }

  turnedOff = true;

  animating = false;
  animatingOff = false;
}

void turnOn() {
  animating = true;
  animatingOff = true;
  int dly = 2048 / currentBrightness;

  turnedOff = false;
  
  for (int i = 0; i <= currentBrightness; i++) {
    LEDS.setBrightness(i);
    showLeds();
    delay(dly);
  }

  animating = false;
  animatingOff = false;
}

void displaySettingValue(int value, uint8_t brightness) {
  // Save state
  animating = true;
  boolean showedTime = showTime;
  uint8_t oldBrightness = LEDS.getBrightness();
  LEDS.setBrightness(brightness);
  //memcpy(ledsBackup, leds, NUM_LEDS * sizeof(CRGB));

  // Clear LEDs and stop time display
  showTime = false;

  memset(leds, 0, sizeof(leds));

  for (int i = 0; i <= value; i++) {
    leds[ledMap[i]] = CRGB::White;
  }

  showLeds();

  settingChanged = true;

  // Restore state
  //memcpy(leds, ledsBackup, NUM_LEDS * sizeof(CRGB));
  LEDS.setBrightness(oldBrightness);
  showTime = showedTime;
  animating = false;
}


void clockTransform() {
  // Basic clock
  if (clockStyle == CLK_STANDARD) {
    secsDisp = secs;
    minsDisp = mins;
    hrsDisp = ((hrs % 12) * 5) + (mins / 12);
  }

  // 24 hour clock
  else if (clockStyle == CLK_24HR) {
    secsDisp = secs;
    minsDisp = mins;
    hrsDisp = (hrs * 2.5) + (mins / 24);
  }

  // Audsley clock (backwards)
  else if (clockStyle == CLK_AUDSLEY) {
    secsDisp = (60 - secs) % 60;
    minsDisp = (60 - mins) % 60;
    hrsDisp = (60 - (((hrs % 12) * 5) + (mins / 12))) % 60;
  }

  // Moving clock face
  else if (clockStyle == CLK_MOVEFACE) {
    secsDisp = 0;
    minsDisp = (mins + secs) % 60;
    hrsDisp = (((hrs % 12) * 5) + (mins / 12) + secs) % 60;
  }

  // NO CLOCK
  else if (clockStyle == CLK_PAUSE) {
    
  }
}


void tickAnimation() {
  // Basic ticking
  if (tickStyle == TICK_STANDARD) {
    leds[ledMap[secsDisp]] += secColour;
    leds[ledMap[minsDisp]] += minColour;
    leds[ledMap[hrsDisp]] += hrColour;

    if (showTime && !turnedOff && !settingChanged)
      showLeds();
  }

  // RESERVED
  else if (tickStyle == TICK_RESERVED) {

  }

  // Pulse each tick
  else if (tickStyle == TICK_PULSE) {
    for (int i = 0; i < currentBrightness; i+=4) {
      memset(leds, 0, sizeof(leds));
      leds[ledMap[secsDisp]] = CRGB(0, 0, i);
      leds[ledMap[minsDisp]] += minColour;
      leds[ledMap[hrsDisp]] += hrColour;
      if (showTime && !turnedOff && !settingChanged)
        showLeds();
    }
    for (int i = currentBrightness; i >= 0; i-=4) {
      memset(leds, 0, sizeof(leds));
      leds[ledMap[secsDisp]] = CRGB(0, 0, i);
      leds[ledMap[minsDisp]] += minColour;
      leds[ledMap[hrsDisp]] += hrColour;
      if (showTime && !turnedOff && !settingChanged)
        showLeds();
    }
  }

  // Spin after each tick
  else if (tickStyle == TICK_SPIN) {
    for (int i = 0; i <= NUM_LEDS; i++) {
      memset(leds, 0, sizeof(leds));
      leds[ledMap[minsDisp]] += minColour;
      leds[ledMap[hrsDisp]] += hrColour;
      leds[ledMap[(secsDisp + i) % NUM_LEDS]] += secColour;
      
      if (showTime && !turnedOff && !settingChanged)
        showLeds();
    }
  }

  // Fill up to current seconds
  else if (tickStyle == TICK_FILL) {
    if (mins % 2 == 1) {
      for (int i = secsDisp; i < NUM_LEDS; i++)
        leds[ledMap[i]] += secColour;
    }
    else {
      for (int i = 0; i <= secsDisp; i++)
        leds[ledMap[i]] += secColour;
    }
    leds[ledMap[minsDisp]] = minColour;
    leds[ledMap[hrsDisp]] = hrColour;

    if (showTime && !turnedOff && !settingChanged)
      showLeds();
  }
}


void showLeds() {
  unsigned long maxPower = LED_POWER * (power + 1);
  unsigned int total = 0;
  uint8_t maxBrightness =  LEDS.getBrightness();

  for (int i = 0; i < NUM_LEDS; i++) {
    total += min(leds[i].r, maxBrightness);
    total += min(leds[i].g, maxBrightness);
    total += min(leds[i].b, maxBrightness);
  }

  // Limit power!
  if (total > maxPower) {
    unsigned int scale = (maxPower * 100l) / (long)total;

    unsigned int rScale;
    unsigned int gScale;
    unsigned int bScale;

    for (int i = 0; i < NUM_LEDS; i++) {
      rScale = leds[i].r * scale;
      gScale = leds[i].g * scale;
      bScale = leds[i].b * scale;

      leds[i].r = rScale / 100;
      leds[i].g = gScale / 100;
      leds[i].b = bScale / 100;
    }
  }

  LEDS.show();
}

