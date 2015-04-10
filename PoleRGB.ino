#include "FastLED.h"
#include "logo.h"
#include <UIPEthernet.h>

EthernetServer server = EthernetServer(1000);

#define CLOCK_PIN 20
#define DATA_PIN 21

#define NUM_PIXELS 72
CRGB pixels[NUM_PIXELS] = {0};

CLEDController *ledController;


void setup() {
  Serial.begin(9600);

  ledController = &FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN>(pixels, NUM_PIXELS);

  uint8_t mac[6] = {0x00,0x01,0x02,0x03,0x04,0x05};
  IPAddress myIP(10,0,0,69);

  Ethernet.begin(mac,myIP);

  server.begin();
}

bool flag=false;
void loop() {

  showImage();

  size_t size;

  if (EthernetClient client = server.available())
    {
      while((size = client.available()) > 0)
        {
          uint8_t* msg = (uint8_t*)malloc(size);
          size = client.read(msg,size);
          Serial.write(msg,size);
          free(msg);
        }
      client.println("DATA from Server!");
      client.stop();
      flag=true;
    }
}



void showImage() {
  unsigned long start = millis();
  for(int col = 0; col < IMAGE_COLUMNS; col++) {
     /*for(int row = 0; row < NUM_PIXELS; row++) {
       strip.setPixelColor(row, pgm_read_dword( &picture[col*IMAGE_ROWS + row]));
       //strip.setPixelColor(row, 0,255,0);
     }
     strip.show();*/
     //if (!flag) memcpy(pixels, &picture[col*IMAGE_ROWS*PIXEL_SIZE], NUM_PIXELS*3);

     CRGB *addr = flag ? (CRGB *)&picture[col*IMAGE_ROWS*PIXEL_SIZE] : pixels;
     ledController->show(addr, NUM_PIXELS, 255);
  }
  unsigned long end = millis();
  ledController->show(pixels,NUM_PIXELS, 255);
  delay(40);
  Serial.print("columns = "); Serial.print(IMAGE_COLUMNS); Serial.print(", time= "); Serial.println((end-start));
}
