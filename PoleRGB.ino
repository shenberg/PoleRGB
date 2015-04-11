#include "FastLED.h"
#include <spi4teensy3.h>
#include <EtherCard.h>
#include "netbuffer.h"

#define CLOCK_PIN 3
#define DATA_PIN 2

#define NUM_PIXELS 90
CRGB pixels[NUM_PIXELS] = {0};

CLEDController *ledController;

void setup() {
  Serial.begin(9600);

  ledController = &FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN>(pixels, NUM_PIXELS);

  net_setup();
}

void loop() {
  net_update();
  show_image();
}

void show_image() {
  unsigned long start = millis();
  for(int col = 0; col < net_image_width(); col++) {
     CRGB *addr = (CRGB *)(net_image() + col*net_image_height()*PIXEL_SIZE);
     delay(2);
     ledController->show(addr, NUM_PIXELS, 255);
  }
  unsigned long end = millis();
  ledController->show(pixels,NUM_PIXELS, 255);
  delay(40);
}
