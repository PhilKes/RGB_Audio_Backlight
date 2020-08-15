#include "fix_fft.h"                                  //library to perfom the Fast Fourier Transform

#include <SPI.h>                                      //SPI library
#include <Wire.h>                                     //I2C library for OLED
#include <Adafruit_GFX.h>                             //graphics library for OLED
#include <Adafruit_SSD1306.h>                         //OLED driver

#define OLED_RESET 4                                  //OLED reset, not hooked up but required by library
Adafruit_SSD1306 display(OLED_RESET);                 //declare instance of OLED library called display

/* OLED <-> Arduino Nano
   VCC - 5V
   GND - GND
   SCK - A5 
   SDA - A4
*/
char im[128], data[128];                              //variables for the FFT
char x = 0, ylim = 30;                                //variables for drawing the graphics
int i = 0, val;                                       //counters

#define DEBUG 1

void setup() {
  #if DEBUG
  Serial.begin(115200);
  #endif
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);          //begin OLED @ hex addy 0x3C
  display.setTextSize(1);                             //set OLED text size to 1 (1-6)
  display.setTextColor(WHITE);                        //set text color to white
  display.clearDisplay();                             //clear display
  analogReference(DEFAULT);                           // Use default (5v) aref voltage.
}

void loop() {

  int min=1024, max=0;                                //set minumum & maximum ADC values
  for (i = 0; i < 128; i++) {                         //take 128 samples
    val = analogRead(A0);                             //get audio from Analog 0
    data[i] = val/4-128 ;                          //each element of array is val/4-128
    im[i] = 0;                                        //
    if(val>max) max=val;                              //capture maximum level
    if(val<min) min=val;                              //capture minimum level
  };
    
  fix_fft(data, im, 7, 0);                            //perform the FFT on data
  
  display.clearDisplay();                             //clear display
  for (i = 1; i < 64; i++) {                          // In the current design, 60Hz and noise
    int dat = sqrt(data[i] * data[i] + im[i] * im[i]);//filter out noise and hum
    display.drawLine(i*2 + x, ylim, i*2 + x, ylim - dat, WHITE); // draw bar graphics for freqs above 500Hz to buffer
    #if DEBUG
    Serial.print(dat);
    Serial.print(" ");
    #endif
  };      
  #if DEBUG            
  Serial.println();
  #endif
  display.display();                                  //show the buffer

}
