
#ifndef _NETBUFFER_H_
#define _NETBUFFER_H_

#include "logo.h"
#include <IPAddress.h>
#include "display.h"

// UDP server port
#define PORT 5000
// due to shitty teensy behavior this needs to be passed to ether::begin
#define CS_PIN 10 

////// TYPES //////

enum packet_type {
	DATA,
	ACK,
	NEW_PIC,
	SHOW_PIC,
	COLOR,
};

struct packet_t {
	uint8_t type;
	uint8_t sequence;
	uint8_t data[0];
} __attribute__ ((packed));

struct data_packet_t {
	uint16_t offset;
	uint16_t size;
	uint16_t total_size;
	uint8_t  data[0];
} __attribute__ ((packed));

struct new_pic_packet_t {
	uint16_t width;
	uint16_t height;
} __attribute__ ((packed));

struct display_mode_packet_t {
	uint32_t delay; // between showing everything once and the next time
	int32_t repeatCount; // 0 for forever
} __attribute__ ((packed));

struct show_pic_packet_t {
	display_mode_packet_t displayParams;
	uint32_t columnDelay;
} __attribute__ ((packed));

struct color_mode_packet_t {
	display_mode_packet_t displayParams;
	uint32_t duration;
	uint8_t r;
	uint8_t g;
	uint8_t b;
} __attribute__ ((packed));


struct image_buffer_t {
	uint16_t width;
	uint16_t height;
	uint8_t data[0];
} __attribute__ ((packed));

///// FUNCTIONS ////

class DoubleBuffer {
public:
	DoubleBuffer() {
		buffers[0] = (image_buffer_t *)malloc(sizeof(image_buffer_t) + 100*90*3);
		buffers[1] = (image_buffer_t *)malloc(sizeof(image_buffer_t) + 100*90*3);
		src = buffers[0];
		dst = buffers[1];
		src_is_zero = false;
		has_image = false;
	}

	~DoubleBuffer() {
		//free(src);
		//free(dst);
	}

	void start_picture(uint16_t width, uint16_t height) {
		uint32_t image_size = PIXEL_SIZE*width*height;
		Serial.print("allocating image, width = "); Serial.print(width); Serial.print(", height = "); Serial.println(height);

		//free(dst);
		//image_buffer_t *buffer = (image_buffer_t *)malloc(sizeof(image_buffer_t) + image_size);
		//buffer->height = height;
		//buffer->width = width;
		dst = buffers[src_is_zero ? 1 : 0];
		dst->width = width;
		dst->height = height;
	}

	void write(const uint8_t *buffer, uint16_t offset, uint16_t size) {
		memcpy(dst->data + offset, buffer, size);
	}

	void done_with_picture() {
		src_is_zero = !src_is_zero;
		src = buffers[src_is_zero ? 0 : 1];
		dst = buffers[src_is_zero ? 1 : 0];
		has_image = true;
	}

	const uint8_t * image() const {
		return has_image ? src->data : picture;
	}

	uint16_t width() const {
		return has_image ? src->width : IMAGE_COLUMNS;
	}

	uint16_t height() const {
		return has_image ? src->height : IMAGE_ROWS;
	}

	void to_display(Display& display) {
		display.setImage(image(), width(), height());
	}

private:
	bool has_image;
	bool src_is_zero;
	image_buffer_t *buffers[2];
	image_buffer_t *src;
	image_buffer_t *dst;
};

DoubleBuffer image_buffer;

bool change_mode = false;

void send_ack(uint8_t sequence) {
	packet_t ack;
	Serial.print("ack to seq: "); Serial.print(sequence); Serial.print(", sizeof: "); Serial.println(sizeof(ack));

	ack.type = ACK;
	ack.sequence = sequence;
    ether.makeUdpReply((char *)&ack, sizeof(ack), PORT);
}


//TODO: assuming packet is valid
void handle_packet(const uint8_t *buffer) {
	const packet_t *header = (const packet_t*)buffer;
	switch(header->type) {
		case DATA: {
			// parse packet data as a data_packet struct
			const data_packet_t *packet = (const data_packet_t *)&header->data[0];
			Serial.print("data: offset="); Serial.print(packet->offset); Serial.print(", size="); Serial.print(packet->size); Serial.print(", total="); Serial.println(packet->total_size);
			image_buffer.write(&packet->data[0], packet->offset, packet->size);
		}
		break;
		case NEW_PIC: {
			Serial.println("new pic");
			const new_pic_packet_t *packet = (const new_pic_packet_t *)&header->data[0];
			image_buffer.start_picture(packet->width, packet->height);
		}
		break;
		case SHOW_PIC: {
			Serial.print("SHOW_PIC: Col delay = ");
			const show_pic_packet_t *packet = (const show_pic_packet_t *)&header->data[0];
			//TODO: take sent data into account
			image_buffer.done_with_picture();
			//TODO: choose primary or secondary display by persistent bit
			Display *display;
			if (packet->displayParams.repeatCount <= 0) {
				display = &Display::getPrimaryDisplay();
				image_buffer.to_display(*display);
			} else {
				display = &Display::getSecondaryDisplay();
				Display::activateSecondaryDisplay();
				image_buffer.to_display(*display);
				image_buffer.done_with_picture(); //TODO: hack! make sure temporary buffer is the buffer used by secondary display even if secondary display is active
			}
			display->setMode(MODE_IMAGE);
			display->setColumnDelay(packet->columnDelay);
			Serial.print(packet->columnDelay); Serial.print(", delay = ");
			display->setDelay(packet->displayParams.delay);
			Serial.print(packet->displayParams.delay); Serial.print(", repeatCount = ");
			display->setRepeatCount(packet->displayParams.repeatCount);
			Serial.println(packet->displayParams.repeatCount);
			//display.debug();
			change_mode = true; //TODO: hack
		}
		break; 
		case COLOR: {
			Serial.println("show color!");
			const color_mode_packet_t *packet = (const color_mode_packet_t *)&header->data[0];
			Display *display;
			if (packet->displayParams.repeatCount == 0) {
				display = &Display::getPrimaryDisplay();
			} else {
				display = &Display::getSecondaryDisplay();
				Display::activateSecondaryDisplay();
			}
			display->setColor(packet->r, packet->g, packet->b);
			display->setMode(MODE_COLOR);
			display->setDuration(packet->duration);
			display->setDelay(packet->displayParams.delay);
			display->setRepeatCount(packet->displayParams.repeatCount);
			change_mode = true;
		}
		break;
		default:
			Serial.println("unrecognized packet received!!!");
			break;
	}
	send_ack(header->sequence);
}

/////////////////// API //////////////////////////////////

// access current image
const uint8_t *net_image() {
	return image_buffer.image();
}

uint16_t net_image_width() {
	return image_buffer.width();
}

uint16_t net_image_height() {
	return image_buffer.height();
}

bool net_changed_mode() {
	return change_mode;
}

// TODO: move/remove
#ifdef TEST_NETWORK
static byte myip[] = { 10, 0, 0, 68 + UNIT_NUMBER };
#else
static byte myip[] = { 192,168,137,200 + UNIT_NUMBER };
#endif
// gateway ip address
//static byte gwip[] = { 192,168,137,1 };
static byte gwip[] = { 10, 0, 0, 138 };
static uint8_t mac[6] = {0x00,0x01,0x02,0x03,0x04,0x05 + UNIT_NUMBER};

byte Ethernet::buffer[1500];

void udp_callback(uint16_t port, byte ip[4], const char *data, uint16_t len) {
  IPAddress src(ip[0], ip[1], ip[2], ip[3]);
  Serial.print("UDP packet from: ");
  Serial.println(src);
  Serial.println(port);
  Serial.println(data);
  Serial.println(len);
  handle_packet((const uint8_t *)data);
}

// call from main setup()
void net_setup() {
// see https://slug.blog.aeminium.org/2014/02/27/using-teensy-3-0-3-1-and-enc28j60-ethernet-module-with-ethercad-library/
// for setup details. this is a custom version of Ethercard, and initialization is different
#if defined(__MK20DX128__) || defined(__MK20DX256__)
  #if F_BUS == 48000000
  spi4teensy3::init(3);
  #elif F_BUS == 24000000
  // spi4teensy3::init(2);
  #endif
#endif
  if (ether.begin(sizeof Ethernet::buffer, mac, CS_PIN) == 0)
    Serial.println( "Failed to access Ethernet controller");
  ether.staticSetup(myip, gwip);

  ether.udpServerListenOnPort(&udp_callback, 5000);

  //TODO: hack, move to different place
  image_buffer.to_display(Display::getPrimaryDisplay());
}

// call from main loop()
void net_update() {
	change_mode = false;
	static uint32_t max_time = 0;
	uint32_t start = millis();
	ether.packetLoop(ether.packetReceive());
	uint32_t end = millis();
	if (end - start > max_time) {
		max_time = end - start;
		Serial.print("new max network time: "); Serial.println(max_time);
	}
}

// wait and provide CPU time to network stack at the same time
bool net_wait(uint32_t how_long) {
  uint32_t end_time = millis() + how_long;
  do {
    net_update();
    if (net_changed_mode()) {
      return true;
    }
  } while (millis() + 1 < end_time);
  // wait last ms with a real wait
  while (millis() < end_time);
  return false;
}


#endif
