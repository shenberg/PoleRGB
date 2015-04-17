
#ifndef _NETBUFFER_H_
#define _NETBUFFER_H_

#include "logo.h"
#include <IPAddress.h>

// UDP server port
#define PORT 5000
// due to shitty teensy behavior this needs to be passed to ether::begin
#define CS_PIN 10 

////// TYPES //////

enum packet_type {
	DATA,
	ACK,
	NEW_PIC,
	SHOW_PIC
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

struct image_buffer_t {
	uint16_t width;
	uint16_t height;
	uint8_t data[0];
} __attribute__ ((packed));


///// FUNCTIONS ////

class DoubleBuffer {
public:
	DoubleBuffer() {
		buffers = new image_buffer_t*[2];
		buffers[0] = buffers[1] = nullptr;
		src = nullptr;
		dst = nullptr;
		src_is_zero = false;
		has_image = false;
	}

	~DoubleBuffer() {
		free(src);
		free(dst);
		delete[] buffers;
	}

	void start_picture(uint16_t width, uint16_t height) {
		uint32_t image_size = PIXEL_SIZE*width*height;
		Serial.print("allocating image, width = "); Serial.print(width); Serial.print(", height = "); Serial.println(height);

		free(dst);
		image_buffer_t *buffer = (image_buffer_t *)malloc(sizeof(image_buffer_t) + image_size);
		buffer->height = height;
		buffer->width = width;
		dst = buffers[src_is_zero ? 1 : 0] = buffer;
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

private:
	bool has_image;
	bool src_is_zero;
	image_buffer_t **buffers;
	image_buffer_t *src;
	image_buffer_t *dst;
};

DoubleBuffer image_buffer;

void send_ack(uint16_t sequence) {
	packet_t ack;
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
			const new_pic_packet_t *packet = (const new_pic_packet_t *)&header->data[0];
			image_buffer.start_picture(packet->width, packet->height);
		}
		break;
		case SHOW_PIC:
			Serial.println("starting to use received image!");
			image_buffer.done_with_picture();
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

// TODO: move/remove
static byte myip[] = { 10,0,0,69 };
// gateway ip address
static byte gwip[] = { 10,0,0,1 };
static uint8_t mac[6] = {0x00,0x01,0x02,0x03,0x04,0x05};

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
}

// call from main loop()
void net_update() {
	ether.packetLoop(ether.packetReceive());

}


#endif
