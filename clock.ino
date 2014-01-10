#include "FastSPI_LED2.h"
#include <LwRx.h>

#define NUM_LEDS 60
#define DATA_PIN 11



// Standard LED map
uint8_t ledMap[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59};
// Backwards LED map
//uint8_t ledMap[] = {59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};

CRGB leds[NUM_LEDS];
const int ledPin = 13;
boolean ledOn = false;
int secs = 0;
int mins = 0;
int hrs = 0;

boolean go = false;

byte msg[10];
byte len = 10;


void setup() {
  // initialize the digital pin as an output.
  pinMode(ledPin, OUTPUT);
  
  Serial.begin(9600);
  
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
  
  LEDS.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);

  initLEDs(32);
  //intro();

  //go = true;
}


ISR(TIMER2_OVF_vect){
  
  if (!go)
    return;

  
  leds[ledMap[secs]] = CRGB::Black;
  leds[ledMap[mins]] = CRGB::Black;
  leds[ledMap[(hrs % 12) * 5]]  = CRGB::Black;

  incSecs();

  leds[ledMap[secs]] += CRGB::Blue;
  leds[ledMap[mins]] += CRGB::Green;
  leds[ledMap[(hrs % 12) * 5]]  += CRGB::Red;

  LEDS.show();
  
  /*
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
  */
  
  //digitalWrite(ledPin, HIGH);
  //delay(20);
  //digitalWrite(ledPin, LOW);
}


void loop() {
  if (lwrx_message()) {
     lwrx_getmessage(msg,&len);
     printMsg(msg, len);

     //if (msg) {
     
     /*
     if (msg[0] == 0xC && msg[1] == 0x0 && msg[2] == 0xF && msg[3] == 0x0) {
       for (int i = 0; i < NUM_LEDS; i++)
         leds[i] = CRGB::Black;
     }
     else {
       int btn = msg[2];
       int type = msg[3];
     
       leds[btn] = (type == 1) ? CRGB::White : CRGB::Black;
     }
     */

    int btn = msg[2];
    int type = msg[3];

    if (btn == 0) {
    	go = type;
    }
    else if (btn == 1) {

			leds[ledMap[secs]] = CRGB::Black;
  		leds[ledMap[mins]] = CRGB::Black;
  		leds[ledMap[(hrs % 12) * 5]]  = CRGB::Black;

    	if (type)
    		incSecs();
    	else
    		decSecs();

	    leds[ledMap[secs]] += CRGB::Blue;
	  	leds[ledMap[mins]] += CRGB::Green;
	  	leds[ledMap[(hrs % 12) * 5]]  += CRGB::Red;
	  
	    LEDS.show();
	  }
  }
}

void initLEDs(byte brightness) {
	LEDS.setBrightness(brightness);
	LEDS.clear();
}

void intro() {
	uint8_t brightness = LEDS.getBrightness();
  int brightnessScale = 256/brightness;
  
  for (int i = 0; i < ((255 * 3) - 1); i++) {
    fill_rainbow(leds, NUM_LEDS, i % 255, 255/NUM_LEDS);
    LEDS.show();
    delay(2);
  }

  for (int i = 0; i < 255; i++) {
    fill_rainbow(leds, NUM_LEDS, i % 255, 255/NUM_LEDS);
    LEDS.show();
    delay(2);
    LEDS.setBrightness(brightness-(i/brightnessScale));
  }

  LEDS.clear();
  
  LEDS.setBrightness(brightness);
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

void printMsg(byte *msg, byte len) {
  Serial.print(millis());
  Serial.print(" ");
  for(int i=0;i<len;i++) {
    Serial.print(msg[i],HEX);
    Serial.print(" ");
  }
  Serial.println();
}

