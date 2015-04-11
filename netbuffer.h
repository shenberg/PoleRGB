
#ifndef _NETBUFFER_H_
#define _NETBUFFER_H_

#include "logo.h"

////// GLOBALS ////

EthernetUDP udp;

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
		uint32_t image_size = 3*width*height;
		Serial.print("allocating image, width = "); Serial.print(width); Serial.print(", height = "); Serial.println(height);

		free(dst);
		image_buffer_t *buffer = (image_buffer_t *)malloc(sizeof(image_buffer_t) + image_size);
		buffer->height = height;
		buffer->width = width;
		dst = buffers[src_is_zero ? 1 : 0] = buffer;
	}

	void write(uint8_t *buffer, uint16_t offset, uint16_t size) {
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
	udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.write((uint8_t *)&ack, sizeof(ack));
    udp.endPacket();
}


//TODO: assuming packet is valid
void handle_packet(uint8_t *buffer) {
	packet_t *header = (packet_t*)buffer;
	switch(header->type) {
		case DATA: {
			// parse packet data as a data_packet struct
			data_packet_t *packet = (data_packet_t *)&header->data[0];
			Serial.print("data: offset="); Serial.print(packet->offset); Serial.print(", size="); Serial.print(packet->size); Serial.print(", total="); Serial.println(packet->total_size);
			//memcpy(backbuffer->data + packet->offset, &packet->data[0], packet->size);
			image_buffer.write(&packet->data[0], packet->offset, packet->size);
		}
		break;
		case NEW_PIC: {
			new_pic_packet_t *packet = (new_pic_packet_t *)&header->data[0];
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

// call from main setup()
void net_setup() {
  int rc = udp.begin(5000);
  Serial.print("initialize UDP result: "); Serial.println(rc);
}

// call from main loop()
void net_update() {
  static uint8_t recv_buffer[1024];
  int packetSize = udp.parsePacket();
  if(packetSize)
  {
  	Serial.print("received UDP packet, size: "); Serial.println(packetSize);
  	Serial.println(udp.read(recv_buffer, sizeof(recv_buffer)));
  	// TODO: validate data

  	handle_packet(recv_buffer);
  	//udp.stop();
  }
}


#endif
