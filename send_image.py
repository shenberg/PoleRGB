#!/usr/bin/python

import sys
import argparse
import socket
import struct

try:
	# windows
	from PIL import Image
except ImportError:
	# osx
	import Image

broadcast=True;

HEIGHT = 90
def process_pixel(r,g,b, a=255):
	# invert white to black
	# if a < 150 or (r > 240 and g > 240 and b > 240):
		# return 0,0,0

	return r/4,g/4,b/4

seq = 0
def send_packet(s, type, data='', max_timeouts=2):
	global seq, broadcast
	broadcast_addr = ('255.255.255.255', 5000)
	dest_addr = ('192.168.137.204', 5000)
	buffer = chr(type) + chr(seq) + data
	ack_ok = False
	timeout_count = 0

	#s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	#s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
	while not ack_ok:
		s.sendto(buffer, dest_addr if not broadcast else broadcast_addr)

		try:
			resp, addr = s.recvfrom(1024)
			if (addr == dest_addr):
				print seq, resp.encode('hex')
				ack_ok = True
			else:
				print "unexpected udp sender: ", addr
		except socket.timeout:
			if timeout_count >= max_timeouts:
				print "too many timeouts!"
				return
			timeout_count += 1
			print "didn't get ack in time, retrying"

	if ord(resp[1]) != seq:
		print "bad ack: seq: {}, expected: {}".format(ord(resp[1]), seq)
	print 'ack ok'
	seq = (seq + 1) % 256

def blocks(seq, size):
	l = []
	for i in xrange(0,len(seq), size):
		l.append(seq[i:i+size])
	return l

def main():
	parser = argparse.ArgumentParser()
	parser.add_argument("image_file", help="Path to an image (gif, jpg, png, etc)")
	parser.add_argument("-r", "--rescale", help="rescale the image proportionaly so that it's 90 pixels high", action="store_true")
	args = parser.parse_args()

	filename = args.image_file
	#rows = 9 # rows = sys.argv[2]

	im = Image.open(filename).convert("RGB")
	if args.rescale and im.size[1] != HEIGHT:
		ratio = float(HEIGHT)/im.size[1]
		im = im.resize((int(round(im.size[0]*ratio)), HEIGHT), Image.ANTIALIAS)
	elif im.size[1] > HEIGHT:
		delta = im.size[1] - HEIGHT
		im = im.crop((0,delta/2, im.size[0], im.size[1] - delta/2))

	cols, rows = im.size
	assert rows == HEIGHT

	data = []
	for column in xrange(cols):
		col_data = []
		for row in xrange(rows):
			# invert the image!
			pixel = im.getpixel((column, HEIGHT - (row + 1)))
			#pixel = im.getpixel((column, row))
			#r,g,b = process_pixel(*pixel)
			col_data.extend(process_pixel(*pixel))
			#val_to_write = b  | (g << 8) | (r << 16)
			#col_data.append(val_to_write)
		data.append(col_data)

	print "sending data to client"
	
	s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	s.settimeout(2)
	s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

	print "sending NEW_PIC (type = 2)"
	send_packet(s, 2, struct.pack('<HH', cols, rows))

	# list of ints, one for each color component
	pic_flat_data = reduce(lambda x,y: x+y, data)
	print "sending data packets"
	offset = 0
	while True:
		for chunk in blocks(pic_flat_data, 1350): # send 900 bytes of image data per packet
			print "sending packet: offset {}, size: {}, total: {}".format(offset, len(chunk), len(pic_flat_data))
			#send_packet(s, 0, struct.pack("<HHH", offset, len(chunk), len(pic_flat_data)) + "".join(map(chr, chunk)))
			send_packet(s, 6, chr(len(chunk)/(HEIGHT*3)) + "".join(map(chr, chunk)))
			offset += len(chunk)
	print "done sending data!"
	#struct.pack params are: delay between showing the image, repeat count, delay between columns of the image
	#send_packet(s, 3, struct.pack("<LLL", 1000,5,2))
	# COLOR packet
	#send_packet(s, 4, struct.pack("<LLLBBB", 0, 0, 0, 255,255,0)) #, struct.pack("<LLL", 1000,5,2))


if __name__=='__main__':
	main()
