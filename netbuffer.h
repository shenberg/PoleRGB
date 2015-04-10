
#ifndef _NETBUFFER_H_
#define _NETBUFFER_H_

#include "logo.h"

EthernetUDP udp;

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


image_buffer_t *backbuffer = nullptr;
bool flag = false;

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
			memcpy(backbuffer->data + packet->offset, &packet->data[0], packet->size);
		}
		break;
		case NEW_PIC: {
			new_pic_packet_t *packet = (new_pic_packet_t *)&header->data[0];
			uint32_t image_size = 3*packet->width*packet->height;
			Serial.print("allocating image, width = "); Serial.print(packet->width); Serial.print(", height = "); Serial.println(packet->height);

			free(backbuffer);
			backbuffer = (image_buffer_t *)malloc(sizeof(image_buffer_t) + image_size);
			backbuffer->height = packet->height;
			backbuffer->width = packet->width;
		}
		break;
		case SHOW_PIC:
			Serial.println("starting to use received image!");
			flag = true; //TODO: hack
			break;
		default:
			Serial.println("unrecognized packet received!!!");
			break;
	}
	send_ack(header->sequence);
}

const uint8_t *net_image() {
	return flag ? backbuffer->data : picture; //TODO: return network buffer if relevant
}
uint16_t net_image_width() {
	return flag ? backbuffer->width : IMAGE_COLUMNS;
}
uint16_t net_image_height() {
	return flag ? backbuffer->height : IMAGE_ROWS;
}


void net_setup() {
  int rc = udp.begin(5000);
  Serial.print("initialize UDP result: "); Serial.println(rc);
}

void net_update() {
  static uint8_t recv_buffer[1024];
  int packetSize = udp.parsePacket();
  if(packetSize)
  {
  	Serial.print("received UDP packet, size: "); Serial.println(packetSize);
  	Serial.println(udp.read(recv_buffer, sizeof(recv_buffer)));
  	// TODO: validate data

  	handle_packet(recv_buffer);
  }
}


#endif
