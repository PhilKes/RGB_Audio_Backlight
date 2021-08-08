#include "fix_fft.h"                                  //library to perfom the Fast Fourier Transform

#include <SPI.h>                                      //SPI library
#include <Wire.h>                                     //I2C library for OLED

#include <Adafruit_NeoPixel.h>

#define LED_DATA 6
#define POT_BRIGHTNESS A2

#define NUM_LEDS 45         // <-- CHANGE TO NUMBER OF LEDS ON YOUR STRIP
#define BTN_PIN 3           // Button on Interrupt Pin 3         
#define BRIGHTNESS 255
#define MAX_MODE 7          // Amount of modes (used in Switch-Case)
#define DELAY 13

#define AUX_IN A1

//Enable Serial DEBUG
#define DEBUG 0

//MINIMUM DELAY FOR BUTTON TO TRIGGER NEXT ISR
#define ISR_DELAY 600
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_DATA, NEO_GRB + NEO_KHZ800);

volatile uint16_t time_since_isr = 0;
volatile boolean breakMode = false;
volatile uint8_t mode = 0;
//  Buffer arrays for Audio Analyzer Mode
volatile int spectrumValue[8];
volatile uint8_t mapValue[8];
volatile uint32_t audioBuffer[NUM_LEDS];
int filter = 0;

volatile uint16_t brightness;

bool countUp = true;
uint32_t lastColor = 0;

//Parallel Output to OLED Display
/* OLED <-> Arduino Nano
   VCC - 5V
   GND - GND
   SCK - A5
   SDA - A4
*/

char im[128], data[128];                              //variables for the FFT
char x = 0, ylim = 30;                                //variables for drawing the graphics
int val = 0;                                     //counters

/* ISR for Button to switch modes */
void changeMode() {
  if (millis() - time_since_isr > ISR_DELAY) {
#if DEBUG
    Serial.println("PRESSED");
#endif
    time_since_isr = millis();
    mode = (mode + 1) % MAX_MODE;
    breakMode = true;
  }
}
void setup() {
#if DEBUG
  Serial.begin(115200);
#endif

  analogReference(DEFAULT);    // Use default (5v) aref voltage.

  // Init Strip with max. Brightness
  brightness = BRIGHTNESS;
  strip.setBrightness(BRIGHTNESS);
  strip.begin();
  strip.show();

  pinMode(BTN_PIN, INPUT);
  pinMode(POT_BRIGHTNESS, INPUT);
  // Attach interrupt to Pin 3 for Push Button to switch modes
  attachInterrupt(1, changeMode, RISING);
  initAudioBuffer();
}


/* Checks if Potentiometer value has changed, sets new Brightness and return true */
boolean checkBrightness() {
  //WS2812 takes value between 0-255
  uint16_t bright = map(constrain(analogRead(POT_BRIGHTNESS), 0, 1024), 0, 1024, 10, 255);
  if (abs(bright - brightness) > 10) {
#if DEBUG
    Serial.print("B");
    Serial.print(bright);
    Serial.println();
#endif
    brightness = bright;
    strip.setBrightness(brightness);
    return true;
  }
  return false;
}


void initAudioBuffer() {
  for (int i = 0; i < NUM_LEDS; i++) {
    audioBuffer[i] = 0;
  }
}

void fftAudio() {
  initAudioBuffer();
  while (!breakMode) {
    checkBrightness();
    if (breakMode)
      return;
    int min = 1024, max = 0;                            //set minumum & maximum ADC values
    for (int i = 0; i < 128; i++) {                         //take 128 samples
      val = analogRead(AUX_IN);                             //get audio from Analog 0
      data[i] = val;                      //each element of array is val/4-128
      im[i] = 0;                                        //
      if (val > max) max = val;                         //capture maximum level
      if (val < min) min = val;                         //capture minimum level
    };

    fix_fft(data, im, 7, 0);                            //perform the FFT on data

    for (int i = 0; i < 8; i++) {
      spectrumValue[i] = 0;
      for (int j = 0; j < 8; j++) {
        int dat = sqrt(data[i * 8 + j] * data[i * 8 + j] + im[i * 8 + j] * im[i * 16 + j]); //filter out noise and hum;
        if (dat > spectrumValue[i])
          spectrumValue[i] = dat;
      }
      mapValue[i] = map(spectrumValue[i], 0, 30, 0, 255);
     
    }
    /*#if DEBUG
        Serial.println();
      #endif*/

    //Shift LED values forward


    for (volatile int i = NUM_LEDS - 1; i > 0; i--) {
      audioBuffer[i] = audioBuffer[i - 1];
    }
    //Uncomment / Comment above to Shift LED values backwards
    //for (int k = 0; k < NUM_LEDS-1; k++) {
    //  audioBuffer[k] = audioBuffer[k + 1];
    //}

    //Load new Audio value to first LED
    //Uses band 0,2,4 (Bass(red), Middle(green), High(blue) Frequency Band)
    //Lowest 8Bit: Blue , Middle Green , Highest Red
    //Use audioBuffer[NUM_LEDS-1] when using LED shift backwards!

    volatile uint8_t r=mapValue[1];
    volatile uint8_t g=mapValue[4];
    volatile uint8_t b=mapValue[5];
    uint8_t dif= 10;

    
    if((r> dif) && (r> g) && (r >b)){
      r=255;
      g=0;
      b=0;
    }
    else if((g> dif) &&(g > r) && (g > b)){
      g=255;
      r=0;
      b=0;
    }
    else if((b> dif) &&(b > r) && (b > r)){
      b=255;
      r=0;
      g=0;
    }
    
    /*if((r - g > dif) && (r - b > dif)){
      r=255;
      g=0;
      b=0;
    }
    else if((g - r> dif) && (g - b >dif)){
      g=255;
      r=0;
      b=0;
    }
    else if((b - r> dif) && (b - r> dif)){
      b=255;
      r=0;
      g=0;
    }*/

    audioBuffer[0] = r; //RED
    audioBuffer[0] = audioBuffer[0] << 16;
    audioBuffer[0] |= ((g ) << 8); //GREEN
    audioBuffer[0] |= (b / 3);     //BLUE

#if DEBUG
    Serial.print(r);
    Serial.print(" ");
    Serial.print(g);
    Serial.print(" ");
    Serial.print(b);
    Serial.println();
#endif


    //Send new LED values to WS2812


    for ( int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, strip.Color(audioBuffer[i] >> 16, (audioBuffer[i] >> 8)&B11111111, audioBuffer[i]&B11111111));
    }
    strip.show();
  }
}

void loop() {
  breakMode = false;
  switch (mode) {
    case 0:
      Serial.println("White");
      plainColor(255, 255, 255);
      break;
    case 1:                        // CHANGE IF NOT USING MSGEQ7
      Serial.println("Music");
      fftAudio();
      break;
    case 2:
      Serial.println("Red");
      plainColor(255, 0, 0);
      break;
    case 3:
      Serial.println("Green");
      plainColor(0, 255, 0);
      break;
    case 4:
      Serial.println("Blue");
      plainColor(0, 0, 255);
      break;
    case 5:
      Serial.println("Raindow");
      rainbowFade2White(400, 255, 10);
      break;
    case 6:
      Serial.println("ColorRoom");
      colorRoom();
      break;
    default:
#if DEBUG
      Serial.println("DEFAULT");
#endif
      break;
  }
}
//Shows plain color along the strip
void plainColor(uint8_t red, uint8_t green, uint8_t blue) {
  for (int i = 0; i < strip.numPixels(); i++)
    strip.setPixelColor( i, strip.Color( red, green, blue ) );
  strip.show();
  while (!breakMode) {
    if (checkBrightness())
      strip.show();
  }
}
// Use Brightness Potentiometer to change Color
void colorRoom() {
  strip.setBrightness(255);
  uint32_t colorNew = map(constrain(analogRead(POT_BRIGHTNESS), 0, 1024), 0, 1024, 0, 255);
  if (abs(colorNew - lastColor) > 10) {
    lastColor = colorNew;
    for (int i = 0; i < strip.numPixels(); i++)
      strip.setPixelColor(i, Wheel((colorNew) & 255));
    strip.show();
  }
  delay(10);
}


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3, 0);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0, 0);
}
//Animation showing fading rainBow color along the strip
void rainbowFade2White(uint8_t wait, int rainbowLoops, int whiteLoops) {
  float fadeMax = 100.0;
  int fadeVal = fadeMax;
  uint32_t wheelVal;
  int redVal, greenVal, blueVal;

  for (int k = 0 ; k < rainbowLoops ; k ++) {
    for (int j = 0; j < 256; j++) {
      for (int i = 0; i < strip.numPixels(); i++) {
        wheelVal = Wheel(((i * 256 / strip.numPixels()) + j) & 255);
        redVal = red(wheelVal) * float(fadeVal / fadeMax);
        greenVal = green(wheelVal) * float(fadeVal / fadeMax);
        blueVal = blue(wheelVal) * float(fadeVal / fadeMax);
        strip.setPixelColor( i, strip.Color( redVal, greenVal, blueVal ) );
      }
      if (breakMode)
        return;
      checkBrightness();
      strip.show();
      delay(wait);
    }
  }
  delay(500);
}

uint8_t red(uint32_t c) {
  return (c >> 16);
}
uint8_t green(uint32_t c) {
  return (c >> 8);
}
uint8_t blue(uint32_t c) {
  return (c);
}
