#include <Arduino.h>


#define DEBUG 0

#define NUM_LEDS 60
#define DATA_PIN 11


// Some LED colour constants (GGRRBB)
#define LED_SET5  0x151500
#define LED_SET15 0x002B00
#define LED_SET60 0x350000

#define LED_5  0x151500
#define LED_15 0x151508
#define LED_60 0x151510


// Button functions
#define BTN_HBRIGHT 0
#define BTN_HCOLOUR 1
#define BTN_TICK    2
#define BTN_CLK     3

#define BTN_MBRIGHT 4
#define BTN_MCOLOUR 5
#define BTN_MTYPE   6
#define BTN_ROTATE  7

#define BTN_POWER   8
#define BTN_CORRECT 9
#define BTN_STROBE  10
#define BTN_RAINBOW 11

#define BTN_HRS     12
#define BTN_MINS    13
#define BTN_SECS    14
#define BTN_AMPM    15


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

#define KEYDELAY 550
#define OFFDELAY 500

uint8_t remoteId[6];
uint8_t lastButton[4];
long lastButtonTime;
long lastOffButtonTime;

byte msg[10];
byte len = 10;


// Animation types
#define ANIM_RAINBOW 0 // Rotating rainbow, fades in and out - lasts ~5.2s
#define ANIM_SPIN    1 // Two spinning single LEDs in opposite directions, one half speed - lasts ~3s
#define ANIM_PULSER  2 // Pulse all LEDs in red - lasts ~0.5s
#define ANIM_AM      3 // Pulse all LEDs white on lower half of clock - lasts ~0.5s
#define ANIM_PM      4 // Pulse all LEDs white on upper half of clock - lasts ~0.5s
#define ANIM_STROBE  5 // Do not use


// Standard LED map
//uint8_t ledMap[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59};
// Backwards LED map
//uint8_t ledMap[] = {59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
// Upside-down LED map
//uint8_t ledMap[] = {29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30};
// Clock map
uint8_t ledMap[] = {48, 28, 9, 49, 29, 10, 50, 30, 11, 51, 31, 12, 52, 32, 13, 53, 33, 14, 54, 34, 15, 55, 35, 16, 56, 36, 17, 57, 37, 18, 58, 38, 19, 59, 39, 0, 40, 20, 1, 41, 21, 2, 42, 22, 3, 43, 23, 4, 44, 24, 5, 45, 25, 6, 46, 26, 7, 47, 27, 8};

CRGB leds[NUM_LEDS];
//CRGB ledsBackup[NUM_LEDS];
int secs = 0;
int mins = 0;
int hrs = 0;

int secsDisp = 0;
int minsDisp = 0;
int hrsDisp = 0;


// User settings

#define NUM_TICK      5
#define TICK_STANDARD 0
#define TICK_RESERVED 1
#define TICK_PULSE    2
#define TICK_SPIN     3
#define TICK_FILL     4

#define NUM_CLK       5
#define CLK_STANDARD  0
#define CLK_24HR      1
#define CLK_AUDSLEY   2
#define CLK_MOVEFACE  3
#define CLK_PAUSE     4

#define NUM_MRKR      5
#define MRKR_EVERY5   0
#define MRKR_EVERY15  1
#define MRKR_EVERY30  2
#define MRKR_60ONLY   3
#define MRKR_NONE     4


#define LED_POWER     768


int handBrightness;
int handColours;
int tickStyle = TICK_STANDARD;
int clockStyle = CLK_STANDARD;
int markerBrightness;
int markerColours;
int markerType = MRKR_EVERY5;
int clockAngle = 0;
int power = 7;


CRGB secColour = CRGB::Blue;
CRGB minColour = CRGB::Green;
CRGB hrColour = CRGB::Red;
CRGB marker5;
CRGB marker15;
CRGB marker30;
CRGB marker60;



boolean go = false;
boolean showTime = false;
boolean timeSet = false;
boolean animating = false;
boolean animatingOff = false;
boolean timeChanged = false;
boolean settingChanged = false;
boolean turnedOff = false;

#define BRIGHTNESS 255

uint8_t maxBrightness = BRIGHTNESS;
uint8_t currentBrightness = BRIGHTNESS;
