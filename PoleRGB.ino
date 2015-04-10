#include "FastLED.h"
//#include "logo.h"
#include <UIPEthernet.h>
#include "netbuffer.h"

#define CLOCK_PIN 3
#define DATA_PIN 2

#define NUM_PIXELS 72
CRGB pixels[NUM_PIXELS] = {0};

CLEDController *ledController;

void setup() {
  Serial.begin(9600);

  ledController = &FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN>(pixels, NUM_PIXELS);

  uint8_t mac[6] = {0x00,0x01,0x02,0x03,0x04,0x05};
  IPAddress myIP(10,0,0,69);

  Ethernet.begin(mac,myIP);
  net_setup();
}

void loop() {
  net_update();
  showImage();
}



void showImage() {
  unsigned long start = millis();
  for(int col = 0; col < net_image_width(); col++) {
     //CRGB *addr = flag ? (CRGB *)(net_image() + col*net_image_height()*PIXEL_SIZE) : pixels;
     CRGB *addr = (CRGB *)(net_image() + col*net_image_height()*PIXEL_SIZE);
     ledController->show(addr, NUM_PIXELS, 255);
  }
  unsigned long end = millis();
  ledController->show(pixels,NUM_PIXELS, 255);
  delay(40);
  //Serial.print("columns = "); Serial.print(IMAGE_COLUMNS); Serial.print(", time= "); Serial.println((end-start));
}
