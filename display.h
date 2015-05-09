
#ifndef _DISPLAY_STATE_H_
#define _DISPLAY_STATE_H_

#include "FastLED.h"

#define PIXEL_SIZE 3
#define NUM_PIXELS 90

#define STREAM_COLUMN_COUNT 5

// SPI constants
#define CLOCK_PIN 3
#define DATA_PIN 2

enum DisplayMode {
	MODE_IMAGE,
	MODE_COLOR,
	MODE_EQUALIZER,
	//MODE_IMMEDIATE,
	MODE_STREAM,
	MODE_OFF
};

// forward declaration to break circular dependency loop
bool net_wait(uint32_t durationMillis);


class StreamBuffer {
	static const int BUFFER_COLUMN_COUNT = STREAM_COLUMN_COUNT*2 + 1;
public:
	StreamBuffer() {
		reset();
	}

	// retval = 0 if columns written successfully
	// retval > 0 is number of columns we need to read before we have enough space
	int write(const uint8_t *columns, uint8_t columnCount) {
		int flatReadIndex = readIndex < writeIndex ? readIndex + BUFFER_COLUMN_COUNT : readIndex;
		int endIndex = writeIndex + columnCount;
		// check if enough room available in the ring buffer
		if ((endIndex > flatReadIndex) && (!firstWrite)) {
			return endIndex - flatReadIndex;
		}

		firstWrite = false;
		for(int i = 0; i < columnCount; i++) {
			memcpy(&buffer[writeIndex*NUM_PIXELS*PIXEL_SIZE], &columns[i*NUM_PIXELS*PIXEL_SIZE], NUM_PIXELS*PIXEL_SIZE);
			writeIndex++;
			if (writeIndex >= BUFFER_COLUMN_COUNT) {
				writeIndex -= BUFFER_COLUMN_COUNT;
			}
		}
		return 0;
	}

	// get pointer to current column, move read ptr forward
	// if queue is empty them return NULL // (previously: return the last read result and leave ptr in place)
	uint8_t *readColumn() {
		uint8_t *column = &buffer[readIndex*NUM_PIXELS*PIXEL_SIZE];

		int next = readIndex+1;
		if (next >= BUFFER_COLUMN_COUNT) {
			next -= BUFFER_COLUMN_COUNT;
		}
		// as long as we didn't catch up to writer, move reader forward
		if (next != writeIndex) {
			readIndex = next;
			return column;
		}
		return column;
	}

	void reset() {
		readIndex = writeIndex = 0;
		firstWrite = true;
	}

private:
	bool firstWrite;
	int readIndex;
	int writeIndex;

	uint8_t buffer[NUM_PIXELS*PIXEL_SIZE*BUFFER_COLUMN_COUNT];
};


class Display {
public:
	Display() : currentMode(MODE_IMAGE), imageBuffer(nullptr), columnDelay(0), frameDelay(0), repeatCount(1)  {

	}

	DisplayMode getMode() {
		return currentMode;
	}

	void setMode(DisplayMode newMode) {
		if (currentMode != newMode) {
			switch (newMode) {
				case MODE_STREAM:
					columnDelay = frameDelay = 0;
					streamBuffer.reset();
					break;
				default:
					// do something
					break;
			}
		}
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
		frameDelay = newDelay;
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

	/*
	void setImmediateBuffer(const uint8_t *buffer) {
		memcpy(column, buffer, sizeof(column));
	}*/

	// blocks until writing is successful
	void writeToStream(const uint8_t *buffer, uint8_t columnCount) {
		int missingSpace = streamBuffer.write(buffer, columnCount);
		if (missingSpace > 0) {
			showColumnsNoNetwork(missingSpace);
			streamBuffer.write(buffer, columnCount); // retry, guaranteed to work (if the system ain't broken)
		}
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
			if (frameDelay > 0) {
				ledController->showColor(CRGB::Black, NUM_PIXELS, 255);
				if (net_wait(frameDelay)) break;
			}
			
			bool shouldQuit = false;
			switch(currentMode) {
				case MODE_IMAGE:
					shouldQuit = showImage();
					break;
				case MODE_COLOR:
					shouldQuit = showColor();
					break;
					/*
				case MODE_IMMEDIATE:
					shouldQuit = showImmediate();
					break;*/
				case MODE_STREAM:
					shouldQuit = showStream();
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
	bool showImage() {
		for(int col = 0; col < imageWidth; col++) {
			CRGB *addr = (CRGB *)(imageBuffer + col*imageHeight*PIXEL_SIZE);
			ledController->show(addr, NUM_PIXELS, 255);

			if (net_wait(columnDelay)) return true;
		}
		return false;
	}

	bool showColor() {
		ledController->showColor(color, NUM_PIXELS, 255);
		uint32_t waitTime = duration == 0 ? 0x7FFFFFFF : duration;
		if (net_wait(duration)) return true;
		return false;
	}

	/*
	bool showImmediate() {
		ledController->show((CRGB *)column, NUM_PIXELS, 255);
		while(!net_wait(20));
		return true;
	}*/

	void showColumnsNoNetwork(int columnCount) {
		for(int i = 0; i < columnCount; i++) {
			ledController->show((CRGB *)streamBuffer.readColumn(), NUM_PIXELS, 255);
			delay(columnDelay);
		}
	}

	bool showStream() {
		// special behavior: this one blocks forever
		while(true) {
			CRGB *currentColumn = (CRGB *)streamBuffer.readColumn();
			if (currentColumn != nullptr) {
				ledController->show(currentColumn, NUM_PIXELS, 255);
			}
			//TODO: hack
			if (net_wait(columnDelay) && ((currentMode != MODE_STREAM) || (&getActiveDisplay() != this))) {
				return true;
			}
		}

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
		//TODO: nullptr here may crash? test this
		Display::ledController = &FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, BGR, DATA_RATE_MHZ(12)>(nullptr, NUM_PIXELS);
		ledController->setDither(DISABLE_DITHER);

		// correction doesn't look better, really, so it's disabled for now
		//ledController->setCorrection(TypicalLEDStrip);
	}

private:
	DisplayMode currentMode;

	// data shared between some modes
	CRGB color;
	uint32_t duration;
	uint32_t frameDelay;
	int32_t repeatCount; // 0 means forever
	int32_t columnDelay;

	// immediate column:
	//uint8_t column[PIXEL_SIZE*NUM_PIXELS];

	// data for equalizer mode
	uint8_t equalizerLevel;
	// data for picture modes
	uint16_t imageWidth;
	uint16_t imageHeight;
	const uint8_t *imageBuffer;


	static bool secondaryDisplayActive;

	static Display primaryDisplay, secondaryDisplay;

	static CLEDController *ledController;

	static StreamBuffer streamBuffer;
};

// ugh, static variables in c++ :(
bool Display::secondaryDisplayActive = false;
Display Display::primaryDisplay, Display::secondaryDisplay;
CLEDController *Display::ledController;
StreamBuffer Display::streamBuffer;

#endif
