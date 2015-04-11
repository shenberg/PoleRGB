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

HEIGHT = 90
def process_pixel(r,g,b, a=255):
	# invert white to black
	# if a < 150 or (r > 240 and g > 240 and b > 240):
		# return 0,0,0

	return r,g,b

seq = 0
def send_packet(s, type, data=''):
	global seq
	dest_addr = ('10.0.0.69', 5000)
	buffer = chr(type) + chr(seq) + data
	ack_ok = False
	while not ack_ok:
		s.sendto(buffer, dest_addr)

		try:
			resp, addr = s.recvfrom(1024)
			if (addr == dest_addr):
				ack_ok = True
			else:
				print "unexpected udp sender: ", addr
		except socket.timeout:
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
	parser.add_argument("-r", "--rescale", help="rescale the image proportionaly so that it's 72 pixels high", action="store_true")
	args = parser.parse_args()

	filename = args.image_file
	#rows = 9 # rows = sys.argv[2]

	im = Image.open(filename).convert("RGB")
	if args.rescale and im.size[1] != HEIGHT:
		ratio = float(HEIGHT)/im.size[1]
		im = im.resize((int(round(im.size[0]*ratio)), HEIGHT), Image.ANTIALIAS)

	cols, rows = im.size
	assert rows == HEIGHT

	data = []
	for column in xrange(cols):
		col_data = []
		for row in xrange(rows):
			pixel = im.getpixel((column, row));
			#r,g,b = process_pixel(*pixel)
			col_data.extend(process_pixel(*pixel))
			#val_to_write = b  | (g << 8) | (r << 16)
			#col_data.append(val_to_write)
		data.append(col_data)

	print "sending data to client"
	
	s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	s.settimeout(2)

	print "sending NEW_PIC (type = 2)"
	send_packet(s, 2, struct.pack('<HH', cols, rows))

	# list of ints, one for each color component
	pic_flat_data = reduce(lambda x,y: x+y, data)
	print "sending data packets"
	offset = 0
	for chunk in blocks(pic_flat_data, 900): # send 900 bytes of image data per packet
		print "sending packet: offset {}, size: {}, total: {}".format(offset, len(chunk), len(pic_flat_data))
		send_packet(s, 0, struct.pack("<HHH", offset, len(chunk), len(pic_flat_data)) + "".join(map(chr, chunk)))
		offset += len(chunk)
	print "done sending data!"
	send_packet(s, 3)


if __name__=='__main__':
	main()
