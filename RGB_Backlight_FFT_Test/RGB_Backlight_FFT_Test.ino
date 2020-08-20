#include "fix_fft.h"                                  //library to perfom the Fast Fourier Transform

#include <SPI.h>                                      //SPI library
#include <Wire.h>                                     //I2C library for OLED
#include <Adafruit_GFX.h>                             //graphics library for OLED
#include <Adafruit_SSD1306.h>                         //OLED driver

#define OLED_RESET 4                                  //OLED reset, not hooked up but required by library
#define AUX_IN A1
#define BUTTON 3

//Enable Serial DEBUG
#define DEBUG 1

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
int i = 0, val;                                       //counters

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
}

void loop() {

  int min = 1024, max = 0;                            //set minumum & maximum ADC values
  for (i = 0; i < 128; i++) {                         //take 128 samples
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
  for (i = 1; i < 64; i++) {                          // In the current design, 60Hz and noise
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
  if (digitalRead(BUTTON))
    Serial.println("BUTTON");
#endif
#if OLED_OUTPUT
  display.display();                                  //show the buffer
#endif


}
