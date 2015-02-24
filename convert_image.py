import sys
from PIL import Image

def process_pixel(r,g,b, a=255):
	# invert white to black
	# if a < 150 or (r > 240 and g > 240 and b > 240):
		# return 0,0,0

	return r/8,g/8,b/8



def main():
	if len(sys.argv) < 2:
		print "Usage: {} <filename>".format(sys.argv[0])
	filename = sys.argv[1]
	#rows = 9 # rows = sys.argv[2]

	im = Image.open(filename)
	cols, rows = im.size

	data = []
	for column in xrange(cols):
		col_data = []
		for row in xrange(rows):
			pixel = im.getpixel((column, row));
			r,g,b = process_pixel(*pixel)
			val_to_write = b  | (g << 8) | (r << 16)
			col_data.append(val_to_write)
		data.append(col_data)

	#data_string = ",\n".join(', '.join(["{2},{1},{0}".format(pixel & 255, (pixel >> 8) & 255, (pixel >> 16) & 255) for pixel in column]) for column in data)
	data_string = ",\n".join(', '.join(map(str, column[::-1])) for column in data)

	print "#define IMAGE_COLUMNS {}".format(cols);
	print "#define IMAGE_ROWS {}".format(rows);
	print "const uint32_t picture[] PROGMEM = {{\n{data}\n}};".format(data=data_string)


if __name__=='__main__':
	main()