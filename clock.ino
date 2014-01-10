#include "FastSPI_LED2.h"
#include <LwRx.h>

#define NUM_LEDS 60
#define DATA_PIN 11

CRGB leds[NUM_LEDS];
const int ledPin = 13;
boolean ledOn = false;
int seconds = 0;
int minutes = 0;
int hours = 0;

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
  TCCR2B = 0x04; //set the prescaler to 64: 32.768kHz / 128 = 1Hz overflow
  TIFR2 = 0x1; //reset timer2 overflow interrupt flag
  TIMSK2 = 0x01; //enable interrupt on overflow
  SMCR = 0x03; //use power-save mode when we sleep
  sei(); //enable all interrupts
  
  // set up LWRX with rx into pin 2
  lwrx_setup(2);
  
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  LEDS.setBrightness(32);
  
  
  for (int i = 0; i < ((255 * 3) - 1); i++) {
    fill_rainbow(leds, NUM_LEDS, i % 255, 255/NUM_LEDS);
    FastLED.show();
    delay(2);
  }

  for (int i = 0; i < 255; i++) {
    fill_rainbow(leds, NUM_LEDS, i % 255, 255/NUM_LEDS);
    FastLED.show();
    delay(2);
    LEDS.setBrightness(32-(i/8));
  }
  

  for (int i = 0; i < NUM_LEDS; i++)
    leds[i] = CRGB::Black;
  
  FastLED.show();

  LEDS.setBrightness(32);
}


ISR(TIMER2_OVF_vect){
  /*
  if (!go)
    return;
  
  for (int i = 0; i < NUM_LEDS; i++)
    leds[i] = (i == seconds) ? CRGB::White : CRGB::Black;
  
  FastLED.show();
  */
  
  seconds++;
  if (seconds == 60) {
    seconds = 0;
    minutes++;
  }
  if (minutes == 60) {
    minutes = 0;
    hours++;
  }
  if (hours == 24) {
    hours = 0;
  }
  
  /*
  if (hours < 10) Serial.print("0");
  Serial.print(hours);
  if (minutes < 10)
    Serial.print(":0");
  else
    Serial.print(":");
  Serial.print(minutes);
  if (seconds < 10)
    Serial.print(":0");
  else
    Serial.print(":");
  Serial.print(seconds);
  Serial.print("\n");
  */
  
  digitalWrite(ledPin, HIGH);
  delay(300);
  digitalWrite(ledPin, LOW);
}


void loop() {
  if (lwrx_message()) {
     lwrx_getmessage(msg,&len);
     printMsg(msg, len);
     
     if (msg[0] == 0xC && msg[1] == 0x0 && msg[2] == 0xF && msg[3] == 0x0) {
       for (int i = 0; i < NUM_LEDS; i++)
         leds[i] = CRGB::Black;
     }
     else {
       int btn = msg[2];
       int type = msg[3];
     
       leds[btn] = (type == 1) ? CRGB::White : CRGB::Black;
     }
  
    FastLED.show();
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

