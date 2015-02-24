#include "SPI.h"
#include "WS2801.h"

// Example to control WS2801-based RGB LED Modules in a strand or strip
// Written by Adafruit - MIT license
/*****************************************************************************/

// Constructor for use with hardware SPI (specific clock/data pins):
WS2801::WS2801(uint16_t n) {
  alloc(n);
  updatePins();
}

// Constructor for use with arbitrary clock/data pins:
WS2801::WS2801(uint16_t n, uint8_t dpin, uint8_t cpin) {
  alloc(n);
  updatePins(dpin, cpin);
}

// Allocate 3 bytes per pixel, init to RGB 'off' state:
void WS2801::alloc(uint16_t n) {
  begun   = false;
  numLEDs = n;
  pixels  = (uint8_t *)calloc(n, 3);
}

// Activate hard/soft SPI as appropriate:
void WS2801::begin(void) {
  if(hardwareSPI == true) {
    startSPI();
  } else {
    pinMode(datapin, OUTPUT);
    pinMode(clkpin , OUTPUT);
  }
  begun = true;
}

// Change pin assignments post-constructor, switching to hardware SPI:
void WS2801::updatePins(void) {
  hardwareSPI = true;
  datapin     = clkpin = 0;
  // If begin() was previously invoked, init the SPI hardware now:
  if(begun == true) startSPI();
  // Otherwise, SPI is NOT initted until begin() is explicitly called.

  // Note: any prior clock/data pin directions are left as-is and are
  // NOT restored as inputs!
}

// Change pin assignments post-constructor, using arbitrary pins:
void WS2801::updatePins(uint8_t dpin, uint8_t cpin) {

  if(begun == true) { // If begin() was previously invoked...
    // If previously using hardware SPI, turn that off:
    if(hardwareSPI == true) SPI.end();
    // Regardless, now enable output on 'soft' SPI pins:
    pinMode(dpin, OUTPUT);
    pinMode(cpin, OUTPUT);
  } // Otherwise, pins are not set to outputs until begin() is called.

  // Note: any prior clock/data pin directions are left as-is and are
  // NOT restored as inputs!

  hardwareSPI = false;
  datapin     = dpin;
  clkpin      = cpin;
  clkport     = portOutputRegister(digitalPinToPort(cpin));
  clkpinmask  = digitalPinToBitMask(cpin);
  dataport    = portOutputRegister(digitalPinToPort(dpin));
  datapinmask = digitalPinToBitMask(dpin);
}

// Enable SPI hardware and set up protocol details:
void WS2801::startSPI(void) {
    SPI.begin();
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);
    //SPI.setClockDivider(SPI_CLOCK_DIV16 );  
    // WS2801 datasheet recommends max SPI clock of 2 MHz, and 50 Ohm
    // resistors on SPI lines for impedance matching.  In practice and
    // at short distances, 2 MHz seemed to work reliably enough without
    // resistors, and 4 MHz was possible with a 220 Ohm resistor on the
    // SPI clock line only.  Your mileage may vary.  Experiment!
     SPI.setClockDivider(SPI_CLOCK_DIV8);  // 4 MHz
}

uint16_t WS2801::numPixels(void) {
  return numLEDs;
}

void WS2801::show(void) {
  uint16_t i, nl3 = numLEDs; // 3 bytes per LED
  uint8_t  bit,t,c;
  //int dm = 1;
  
  // Write 24 bits per pixel:
  if(hardwareSPI) 
  { 
	for(t=0;t<4;t++) //first by sending 32 low bits
		SPI.transfer(0x00);
    for(i=0; i<nl3; i++) 
	{
		SPI.transfer(0xFF);
		SPI.transfer(pixels[3*i]); // blue
		SPI.transfer(pixels[3*i+1]); // blue
		SPI.transfer(pixels[3*i+2]); // blue
    }
	
	for(t=0;t<4;t++) //end by sending 32 high bits
		SPI.transfer(0xFF);
	  
  } 
  else 
  {
	  *dataport &= ~datapinmask;
	for(t=0;t<32;t++)
	{
		*clkport |=  clkpinmask;
		
        *clkport &= ~clkpinmask;
	}
    for(i=0; i<nl3; i++ ) 
	{
		*dataport |=  datapinmask;
		for(t=0;t<8;t++)
		{
			*clkport |=  clkpinmask;
			*clkport &= ~clkpinmask;
		}
		for(c=0;c<3;c++)
		{
			for(bit=0x80; bit; bit >>= 1) 
			{
				if(pixels[i*3+c] & bit) *dataport |=  datapinmask;
				else                *dataport &= ~datapinmask;
				*clkport |=  clkpinmask;
				
				*clkport &= ~clkpinmask;
			}
		}
	}
	*dataport |=  datapinmask;
	for(t=0;t<32;t++)
	{
		*clkport |=  clkpinmask;
        *clkport &= ~clkpinmask;
	}
  }
   

 // delay(1); // Data is latched by holding clock pin low for 1 millisecond
}

void WS2801::showProgmem(uint16_t address) {
  uint16_t i, nl3 = numLEDs; // 3 bytes per LED
  uint8_t  bit,t,c;
  //int dm = 1;
  
  // Write 24 bits per pixel:
  for(t=0;t<4;t++) //first by sending 32 low bits
    SPI.transfer(0x00);
    for(i=0; i<nl3; i++) 
  {
    SPI.transfer(0xFF);
    SPI.transfer(pgm_read_byte(address + 3*i)); // blue
    SPI.transfer(pgm_read_byte(address + 3*i+1)); // blue
    SPI.transfer(pgm_read_byte(address + 3*i+2)); // blue
    }
  
  for(t=0;t<4;t++) //end by sending 32 high bits
    SPI.transfer(0xFF);

 // delay(1); // Data is latched by holding clock pin low for 1 millisecond
}


// Set pixel color from separate 8-bit R, G, B components:
void WS2801::setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
  if(n < numLEDs) { // Arrays are 0-indexed, thus NOT '<='
    uint8_t *p = &pixels[n * 3];
    *p++ = b;
    *p++ = g;
    *p++ = r;
  }
}

// Set pixel color from 'packed' 32-bit RGB value:
void WS2801::setPixelColor(uint16_t n, uint32_t c) {
  if(n < numLEDs) { // Arrays are 0-indexed, thus NOT '<='
    uint8_t *p = &pixels[n * 3];
    *p++ = c >> 16;
    *p++ = c >>  8;
    *p++ = c;
  }
}

