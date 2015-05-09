
//NUMBER OF THIS TEENSY - MATCH THE NUMBER WRITTEN ON THE BOX!!
#define UNIT_NUMBER 4
// if TEST_NETWORK, IP address is 10.0.0.68 + UNIT_NUMBER
// otherwise 192.168.137.200 + UNIT_NUMBER
//#define TEST_NETWORK

#include "FastLED.h"
#include <spi4teensy3.h>
#include <EtherCard.h>
#include "display.h"
#include "netbuffer.h"

//CLEDController *ledController;

void setup() {
  //Serial.begin(9600);

  //ledController = &FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, BGR, DATA_RATE_MHZ(12)>(pixels, NUM_PIXELS);
  //ledController->setDither(DISABLE_DITHER);

  // IMPORTANT: Display setup MUST happen before net_setup() due to ugly interdependency between the modules

  Display::setup();

  net_setup();
}

void loop() {
  //show_image();
  Display& current = Display::getActiveDisplay();
  current.show();
  /*
  switch (current.getMode()) {
    case MODE_IMAGE:
      //Serial.println("Image Mode");
      show_image(current.getImage(), current.getWidth(), current.getHeight(), 0);
      break;

    case MODE_COLOR:
      //Serialn.println("Color Mode");
      show_color(CRGB::Green);
      break;

    case MODE_EQUALIZER:
      //Serial.println("Equalizer Mode");
      show_equalizer(70);
      break;

    case MODE_OFF:
      show_off();
      delay(20);
      break;
  }
  */
}



/*
void show_image(const uint8_t *data, uint16_t width, uint16_t height, uint32_t delay) {
  for(int col = 0; col < width; col++) {
    CRGB *addr = (CRGB *)(data + col*height*PIXEL_SIZE);
    ledController->show(addr, NUM_PIXELS, 255);

    net_update(); // give some CPU time to network processing
    if (net_changed_mode()) {
      return;
    }
  }
  //ledController->show(pixels,NUM_PIXELS, 255);
  //delay(40);
}

void show_off() {
  ledController->showColor(CRGB::Black, NUM_PIXELS, 255);
  net_update();
}

void show_equalizer(uint8_t level) {
  //TODO: this
  ledController->showColor(CRGB::Blue, NUM_PIXELS, 255);
}

void show_color(CRGB color) {
  //TODO: !!!
  ledController->showColor(color, NUM_PIXELS, 255);
  net_update();
}
*/

