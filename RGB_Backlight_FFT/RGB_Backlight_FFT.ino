#include "fix_fft.h"                                  //library to perfom the Fast Fourier Transform

#include <SPI.h>                                      //SPI library
#include <Wire.h>                                     //I2C library for OLED
#include <Adafruit_GFX.h>                             //graphics library for OLED
#include <Adafruit_SSD1306.h>                         //OLED driver

#include <Adafruit_NeoPixel.h>

#define LED_DATA 6

#define NUM_LEDS 45         // <-- CHANGE TO NUMBER OF LEDS ON YOUR STRIP
#define BTN_PIN 3           // Button on Interrupt Pin 3         
#define BRIGHTNESS 255
#define MAX_MODE 7          // Amount of modes (used in Switch-Case)
#define DELAY 13

#define OLED_RESET 4                                  //OLED reset, not hooked up but required by library
#define AUX_IN A1

//Enable Serial DEBUG
#define DEBUG 1

//MINIMUM DELAY FOR BUTTON TO TRIGGER NEXT ISR
#define ISR_DELAY 400
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_DATA, NEO_GRB + NEO_KHZ800);

byte neopix_gamma[] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
  2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
  10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
  17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
  25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
  37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
  51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
  69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
  90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
  115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
  144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
  177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
  215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255
};

volatile uint16_t time_since_isr = 0;
volatile boolean breakMode = false;
volatile uint8_t mode = 0;
//  Buffer arrays for Audio Analyzer Mode
volatile int spectrumValue[8];
volatile uint8_t mapValue[8];
volatile uint32_t audioBuffer[45];
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
#define OLED_OUTPUT 1

#if OLED_OUTPUT
Adafruit_SSD1306 display(OLED_RESET);                 //declare instance of OLED library called display
#endif

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

#if OLED_OUTPUT
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);          //begin OLED @ hex addy 0x3C
  display.setTextSize(1);                             //set OLED text size to 1 (1-6)
  display.setTextColor(WHITE);                        //set text color to white
  display.clearDisplay();                             //clear display
#endif
  analogReference(DEFAULT);    // Use default (5v) aref voltage.

  // Init Strip with max. Brightness
  brightness = BRIGHTNESS;
  strip.setBrightness(BRIGHTNESS);
  strip.begin();
  strip.show();

  pinMode(BTN_PIN, INPUT);

  // Attach interrupt to Pin 3 for Push Button to switch modes
  attachInterrupt(1, changeMode, RISING);
  initAudioBuffer();
}

void initAudioBuffer() {
  for (int i = 0; i < NUM_LEDS; i++) {
    audioBuffer[i] = 0;
  }
}

void fftDebug() {
  initAudioBuffer();
  while (!breakMode) {
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

#if OLED_OUTPUT
    display.clearDisplay();                             //clear display
#endif
    for (int i = 1; i < 64; i++) {                          // In the current design, 60Hz and noise
      int dat = sqrt(data[i] * data[i] + im[i] * im[i]);//filter out noise and hum

#if OLED_OUTPUT
      display.drawLine(i * 2 + x, ylim, i * 2 + x, ylim - dat, WHITE); // draw bar graphics for freqs above 500Hz to buffer
#endif
#if DEBUG
      Serial.print(dat);
      Serial.print(" ");
#endif
    };
#if DEBUG
    Serial.println();
#endif
#if OLED_OUTPUT
    display.display();                                  //show the buffer
#endif

    for (int i = 0; i < 8; i++) {
      spectrumValue[i] = 0;
      for (int j = 0; j < 8; j++) {
        int dat = sqrt(data[i * 8 + j] * data[i * 8 + j] + im[i * 8 + j] * im[i * 16 + j]); //filter out noise and hum;
        if (dat > spectrumValue[i])
          spectrumValue[i] = dat;
      }
      mapValue[i] = map(spectrumValue[i], 0, 30, 0, 255);
#if DEBUG
      Serial.print(mapValue[i]);
      Serial.print(" ");
#endif
    }
#if DEBUG
    Serial.println();
#endif

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

    audioBuffer[0] = mapValue[5]; //RED
    audioBuffer[0] = audioBuffer[0] << 16;
    audioBuffer[0] |= ((mapValue[2] / 2) << 8); //GREEN
    audioBuffer[0] |= (mapValue[4] / 4);       //BLUE
    //Send new LED values to WS2812


    for ( int i = 0; i < 42; i++) {
      strip.setPixelColor(i, strip.Color(audioBuffer[i] >> 16, (audioBuffer[i] >> 8)&B11111111, audioBuffer[i]&B11111111));
    }
    strip.show();
  }
}

void loop() {
  breakMode = false;
  switch (mode) {
    case 0:
      fftDebug();
      break;

    default:
#if DEBUG
      Serial.println("DEFAULT");
#endif
      break;
  }

}
