#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

/** THIS SKETCH HAS ONLY GOT THE ADUIO VISUALIZER MODE USING THE MSGEQ7
    FOR OTHER MODES SEE THE "RGBStripe_Control_WS2812.ino SKETCH        **/

#define USES_POTENTIOMETER 0 // SET THIS TO "1" IF YOU ARE USING POTENTIOMETER FOR BRIGHTNESS CONTROL!
#define POT_BRIGHTNESS A2

#define LED_DATA 6
#define NUM_LEDS 45
#define BRIGHTNESS 255
#define DELAY 13

#define MSGEQ_OUT A1
#define STROBE 2
#define RESET 5

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_DATA, NEO_GRB + NEO_KHZ800);

int spectrumValue[8];
uint8_t mapValue[8];
uint32_t audioBuffer[NUM_LEDS];
int filter = 0;
#if USES_POTENTIOMETER
  uint16_t brightness;
#endif

void setup() {
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
#if defined (__AVR_ATtiny85__)
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif
#if USES_POTENTIOMETER
  brightness = BRIGHTNESS;
#endif
  strip.setBrightness(BRIGHTNESS);
  strip.begin();
  strip.show();

  Serial.begin(9600);
  Serial.println("START");
  pinMode(MSGEQ_OUT, INPUT);
  pinMode(STROBE, OUTPUT);
  pinMode(RESET, OUTPUT);
  
  digitalWrite(RESET, LOW);
  digitalWrite(STROBE, HIGH);

#if USES_POTENTIOMETER
  pinMode(POT_BRIGHTNESS, INPUT);
#endif
}

void loop() {
  musicAnalyzer();
}

#if USES_POTENTIOMETER
  /* Checks if Potentiometer value has changed, sets new Brightness and return true */
  boolean checkBrightness(){
    //WS2812 takes value between 0-255
    uint16_t bright = map(constrain(analogRead(POT_BRIGHTNESS),0,1024), 0, 1024, 10, 255);
    if (abs(bright-brightness)>10){
      brightness = bright;
      strip.setBrightness(brightness);
      return true;
    }
    return false;
  }
#endif

void musicAnalyzer(){
  while(true){
    #if USES_POTENTIOMETER
      checkBrightness();
    #endif
    //Reset MSGEQ
    digitalWrite(RESET, HIGH);
    digitalWrite(RESET, LOW);
    //Read all 8 Audio Bands
    for (int i = 0; i < 8; i++){
      digitalWrite(STROBE, LOW);
      delayMicroseconds(30);
      spectrumValue[i] = analogRead(MSGEQ_OUT);
      spectrumValue[i] = constrain(spectrumValue[i], filter, 1023);
      digitalWrite(STROBE, HIGH);
      //Skip band 3 (seems to be random for my MSGEQ7, but feel free to check yours
      if (i == 3)
        continue;
      mapValue[i] = map(spectrumValue[i], filter, 1023, 0, 255);
      if (mapValue[i] < 50)
        mapValue[i] = 0;
    }
    //Shift LED values forward
    for (int k = NUM_LEDS - 1; k > 0; k--) {
      audioBuffer[k] = audioBuffer[k - 1];
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
    audioBuffer[0] |= ((mapValue[2] /2) << 8);  //GREEN
    audioBuffer[0] |= (mapValue[4]/4);         //BLUE
    
    //Send new LED values to WS2812
    for ( int i = 0; i < NUM_LEDS; i++)
      strip.setPixelColor(i, strip.Color(audioBuffer[i] >> 16, (audioBuffer[i] >> 8)&B11111111, audioBuffer[i]&B11111111));
    strip.show();
    delay(DELAY);
  }
}
