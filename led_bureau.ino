#include <FastLED.h>

#define LED_PIN 5
#define NUM_LEDS 96
#define BRIGHTNESS 64
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#include <Keypad.h>
#include <Bounce2.h>
#define ON_OFF_PIN 4

unsigned long BLUE_CORRECTION = 70;
unsigned long GREEN_CORRECTION = 130;
unsigned long RED_CORRECTION = 0;

unsigned int speed = 50;

CRGB leds[NUM_LEDS];

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char keys[ROWS][COLS] = {
  {0x1, 0x2, 0x3, 0x4},
  {0x5, 0x6, 0x7, 0x8},
  {0x9, 0xA, 0xB, 0xC},
  {0xD, 0xE, 0xF, 0x10}
};
byte rowPins[ROWS] = {10, 11, 12, 13}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {9, 8, 7, 6}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

Bounce debouncer = Bounce();

bool status = true;
char mode = 0x2;

int iterator = 0;

unsigned long correction = (RED_CORRECTION << 16) | (GREEN_CORRECTION << 8) | (BLUE_CORRECTION);

unsigned long applyCorrection(unsigned long color) {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8)  & 0xFF;
    uint8_t b =  color        & 0xFF;

    uint8_t rc = RED_CORRECTION;
    uint8_t gc = GREEN_CORRECTION;
    uint8_t bc = BLUE_CORRECTION;

    r = (r > rc) ? (r - rc) : r;
    g = (g > gc) ? (g - gc) : g;
    b = (b > bc) ? (b - bc) : b;

    return ((unsigned long)r << 16) | ((unsigned long)g << 8) | b;
}

unsigned long HueToRGB(float h, float s, float v) {
	double r, g, b;

  int ti = (int)floor(h/60) % 6;

  float f = h/60 - ti;

  float x = v * (1.0 - (ti & 1 ? f : 1-f)*s);
  float l = v * (1-s);


	switch (ti)
	{
	  case 0:
        r = v;
        g = x;
        b = l;
        break;
    case 1:
        r = x;
        g = v;
        b = l;
        break;
    case 2:
        r = l;
        g = v;
        b = x;
        break;
    case 3:
        r = l;
        g = x;
        b = v;
        break;
    case 4:
        r = x;
        g = l;
        b = v;
        break;
    case 5:
        r = v;
        g = l;
        b = x;
        break;
	}

	unsigned long red = ((r*255) > RED_CORRECTION) ? ((r*255) - (RED_CORRECTION)) : ((r*255));
	unsigned long green = ((g*255) > GREEN_CORRECTION) ? ((g*255) - (GREEN_CORRECTION)) : ((g*255));
	unsigned long blue = ((b*255) > BLUE_CORRECTION) ? ((b*255) - (BLUE_CORRECTION)) : ((b*255));

	return (red << 16) | (green << 8) | blue;
}

void setup() {
  pinMode(ON_OFF_PIN, OUTPUT);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  Serial.begin(9600);
  debouncer.attach(0, INPUT_PULLUP);
  debouncer.interval(25);
}

void loop() {
  if (status == true) {
    digitalWrite(ON_OFF_PIN, HIGH);
  }

  char key = keypad.getKey();

  if (key != NO_KEY){
    Serial.println((int)mode);
    mode = key;
  }

  //Allumage et extinction
  if (mode == 0x1){
    if (status == true){
      status = false;
      digitalWrite(ON_OFF_PIN, LOW);
      mode = 0x0;
    }
    else{
      status = true;
      digitalWrite(ON_OFF_PIN, HIGH);
      mode = 0x0;
    }
  }

  //Orange
  if (mode == 0x2){   
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = applyCorrection(0xFFA500);
    }
  }

  //Violet
  if (mode == 0x3){
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = applyCorrection(0xFA0FFA);
    }
  }

  //Bicolor moitié orange moitié bleu
  if (mode == 0x4){
    for (int i = 0; i < floor(NUM_LEDS/2); i++) {
      leds[i] = applyCorrection(0xFFA500);
    }
    for (int i = ceil(NUM_LEDS/2); i < NUM_LEDS; i++) {
      leds[i] = applyCorrection(0x0000FF); // Set all LEDs to blue
    }
  }

  //Bicolor orange/bleu 4 led sur 8
  if (mode == 0x5){
    for (int i = 0; i < NUM_LEDS; i++) {
      if ((i%8) > 4){
        leds[i] = applyCorrection(0x0000FF);
      }
      else{
        leds[i] = applyCorrection(0xFFA500);
      }
    }
  }

  //Arc en ciel
  if (mode == 0x6){
    for (int i = 0; i < NUM_LEDS; i++) {
      float hue = i*(360 / (NUM_LEDS + 1));
      leds[i] = HueToRGB(hue, 1, 1);
    }

    mode = 0x10;
  }

  //Blanc
  if (mode == 0x7){
    unsigned long val = applyCorrection(0xFFFFFF);
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = val;
    }
  }

  //Ralentissement de l'animation
  if (mode == 0xE){
    if (speed < 0xFFFF){
      speed += 1;
    }
    mode = 0x10;
  }

  //Acceleration de l'animation
  if (mode == 0xF){
    if (speed > 0){
      speed -= 1;
    }
    mode = 0x10;
  }

  //Mode animation
  if (mode == 0x10){
    CRGB color = leds[NUM_LEDS - 1];
    
    for (int i = NUM_LEDS - 2; i >= 0; i--) {
      leds[i+1] = leds[i];
    }

    leds[0] = color;
    delay(speed);
  }
  
  FastLED.show();
  
}