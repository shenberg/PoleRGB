
#ifndef _DISPLAY_STATE_H_
#define _DISPLAY_STATE_H_

#include "FastLED.h"

enum DisplayMode {
	MODE_IMAGE,
	MODE_COLOR,
	MODE_EQUALIZER,
	MODE_OFF
};

class Display {
public:
	Display() : currentMode(MODE_OFF), imageBuffer(nullptr) {

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

private:
	DisplayMode currentMode;

	// data shared between some modes
	CRGB color;
	uint32_t duration;
	uint32_t delay;
	int32_t repeatCount; // -1 means forever

	// data for equalizer mode
	uint8_t equalizerLevel;
	// data for picture modes
	uint16_t imageWidth;
	uint16_t imageHeight;
	const uint8_t *imageBuffer;


	static bool secondaryDisplayActive;

	static Display primaryDisplay, secondaryDisplay;
};

// ugh, static variables in c++ :(
bool Display::secondaryDisplayActive = false;
Display Display::primaryDisplay, Display::secondaryDisplay;

#endif
