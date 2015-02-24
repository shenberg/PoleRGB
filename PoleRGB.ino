#include "SPI.h"
#include "WS2801.h"

#include "logo.h"

#define NUM_PIXELS 72
// Set the first variable to the NUMBER of pixels. 25 = 25 pixels in a row
WS2801 strip = WS2801(NUM_PIXELS);

void setup() {
  strip.begin();
  strip.show();

  Serial.begin(9600);
}

void loop() {
 #if 0
  // Some example procedures showing how to display to the pixels:
  colorWipe(strip.Color(255, 0, 0), 50); // Red
  colorWipe(strip.Color(0, 255, 0), 50); // Green
  colorWipe(strip.Color(0, 0, 255), 50); // Blue
  // Send a theater pixel chase in...
  theaterChase(strip.Color(127, 127, 127), 50); // White
  theaterChase(strip.Color(127,   0,   0), 50); // Red
  theaterChase(strip.Color(  0,   0, 127), 50); // Blue

  rainbow(20);
  rainbowCycle(20);
  theaterChaseRainbow(50);
 #endif
// updateState();
// stateToPixels(5);
  showImage();

  for(int i = 0; i > NUM_PIXELS; i++) {
    strip.setPixelColor(i, 0);
  }
  strip.show();
  delay(40);
}



void showImage() {
  for(int col = 0; col < IMAGE_COLUMNS; col++) {
     for(int row = 0; row < NUM_PIXELS; row++) {
       strip.setPixelColor(row, pgm_read_dword( &picture[col*IMAGE_ROWS + row]));
       //strip.setPixelColor(row, 0,255,0);
     }
     strip.show();
     delay(3);
  }
}
