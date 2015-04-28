
#ifndef _DISPLAY_STATE_H_
#define _DISPLAY_STATE_H_

#include "FastLED.h"

#define PIXEL_SIZE 3
#define NUM_PIXELS 90

// SPI constants
#define CLOCK_PIN 3
#define DATA_PIN 2

enum DisplayMode {
	MODE_IMAGE,
	MODE_COLOR,
	MODE_EQUALIZER,
	MODE_OFF
};

// forward declaration to break circular dependency loop
bool net_wait(uint32_t durationMillis);

CRGB pixels[NUM_PIXELS] = {0};

class Display {
public:
	Display() : currentMode(MODE_IMAGE), imageBuffer(nullptr), columnDelay(0), delay(0), repeatCount(1)  {

	}

	DisplayMode getMode() {
		return currentMode;
	}

	void setMode(DisplayMode newMode) {
		currentMode = newMode;
	}

	CRGB getColor() {
		return color;
	}

	void setColor(byte r, byte g, byte b) {
		color.r = r;
		color.g = g;
		color.b = b;
	}

	void setDuration(uint32_t milliseconds) {
		duration = milliseconds;
	}

	void setDelay(uint32_t newDelay) {
		delay = newDelay;
	}

	void setEqualizerLevel(uint8_t level) {
		equalizerLevel = level;
	}

	void setRepeatCount(int32_t count) {
		repeatCount = count;
	}

	void setColumnDelay(uint32_t delay) {
		columnDelay = delay;
	}

	void setImage(const uint8_t *buffer, uint16_t width, uint16_t height) {
		imageWidth = width;
		imageHeight = height;
		imageBuffer = buffer;
	}

	const uint8_t *getImage() {
		return imageBuffer;
	}

	uint16_t getWidth() {
		return imageWidth;
	}

	uint16_t getHeight() {
		return imageHeight;
	}

	void debug() {
		ledController->showColor(repeatCount, NUM_PIXELS);
		net_wait(4000);
	}


	void show() {
		int32_t maxIteration = repeatCount > 0 ? repeatCount : ((1<<31) - 1);
		/*
		if (repeatCount == 0) {
			ledController->showColor(CRGB::Blue, NUM_PIXELS, 255);
			net_wait(4000);			
		} else if (repeatCount < 0) {
			ledController->showColor(CRGB::Red, NUM_PIXELS, 255);
			net_wait(4000);						
		}*/
		for(int32_t i = 0; i < maxIteration; i++) {
			ledController->showColor(CRGB::Black, NUM_PIXELS, 255);
			if (net_wait(delay)) break;
			bool shouldQuit = false;
			switch(currentMode) {
				case MODE_IMAGE:
					shouldQuit = show_image();
					break;
				case MODE_COLOR:
					shouldQuit = show_color();
					break;
			}
			/*
			// if we aren't leaving and not in the last iteration, do a delay
			if ((!shouldQuit) && (i != maxIteration)) {
				ledController->showColor(CRGB::Black, NUM_PIXELS, 255);
				shouldQuit = net_wait(delay);
			}*/
			if (shouldQuit) break;
		}
	}

	// show_* functions return true if we need to change modes
	bool show_image() {
		for(int col = 0; col < imageWidth; col++) {
			CRGB *addr = (CRGB *)(imageBuffer + col*imageHeight*PIXEL_SIZE);
			ledController->show(addr, NUM_PIXELS, 255);

			if (net_wait(columnDelay)) return true;
		}
		return false;
	}

	bool show_color() {
		ledController->showColor(color, NUM_PIXELS, 255);
		uint32_t waitTime = duration == 0 ? 0x7FFFFFFF : duration;
		if (net_wait(duration)) return true;
		return false;
	}
	//// global state access ///////

	static Display& getPrimaryDisplay() {
		return primaryDisplay;
	}
	static Display& getSecondaryDisplay() {
		return secondaryDisplay;
	}

	static void activateSecondaryDisplay() {
		secondaryDisplayActive = true;
	}

	static Display& getActiveDisplay() {
		if (secondaryDisplayActive) {
			secondaryDisplayActive = false;
			return secondaryDisplay;
		}
		return primaryDisplay;
	}

	static void setup() {
		Display::ledController = &FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, BGR, DATA_RATE_MHZ(12)>(pixels, NUM_PIXELS);
		ledController->setDither(DISABLE_DITHER);

		// correction doesn't look better, really, so it's disabled for now
		//ledController->setCorrection(TypicalLEDStrip);
	}

private:
	DisplayMode currentMode;

	// data shared between some modes
	CRGB color;
	uint32_t duration;
	uint32_t delay;
	int32_t repeatCount; // 0 means forever
	int32_t columnDelay;

	// data for equalizer mode
	uint8_t equalizerLevel;
	// data for picture modes
	uint16_t imageWidth;
	uint16_t imageHeight;
	const uint8_t *imageBuffer;


	static bool secondaryDisplayActive;

	static Display primaryDisplay, secondaryDisplay;

	static CLEDController *ledController;
};

// ugh, static variables in c++ :(
bool Display::secondaryDisplayActive = false;
Display Display::primaryDisplay, Display::secondaryDisplay;
CLEDController *Display::ledController;

#endif
