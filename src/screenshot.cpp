#include "screenshot.h"

#include <X11/Xutil.h>
#include <vector>


#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <jpeglib.h>

static int counter = 0;

int write_jpeg2(XImage *img, const char* filename, 	std::vector<unsigned char> bg_red, std::vector<unsigned char> bg_green, std::vector<unsigned char> bg_blue)
{
	FILE* outfile;
	int x, y;
	char* buffer;
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr       jerr;
	JSAMPROW row_pointer;

	outfile = fopen(filename, "wb");
	if (!outfile) {
		fprintf(stderr, "Failed to open output file %s", filename);
		return 1;
	}

	const unsigned long red_mask = img->red_mask;
	const unsigned long green_mask = img->green_mask;
	const unsigned long blue_mask = img->blue_mask;


	/* collect separate RGB values to a buffer */
	buffer = (char*)malloc(sizeof(char)*3*img->width*img->height);
	for (y = 0; y < img->height; y++) {
		for (x = 0; x < img->width; x++) {

			const int pid = y*img->width + x;
			unsigned long pixel = XGetPixel(img, x, y);
			unsigned char blue = pixel & blue_mask;
			unsigned char green = (pixel & green_mask) >> 8;
			unsigned char red = (pixel & red_mask) >> 16;

			red = std::abs(red - bg_red[pid]);
			green = std::abs(green - bg_green[pid]);
			blue = std::abs(blue - bg_blue[pid]);

			buffer[y*img->width*3+x*3+0] = (char)red;
			buffer[y*img->width*3+x*3+1] = (char)green;
			buffer[y*img->width*3+x*3+2] = (char)blue;


			/*
			pixel = XGetPixel(img,x,y);
			buffer[y*img->width*3+x*3+0] = (char)(pixel>>16);
			buffer[y*img->width*3+x*3+1] = (char)((pixel&0x00ff00)>>8);
			buffer[y*img->width*3+x*3+2] = (char)(pixel&0x0000ff);
			*/
		}
	}

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, outfile);

	cinfo.image_width = img->width;
	cinfo.image_height = img->height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, 85, TRUE);
	jpeg_start_compress(&cinfo, TRUE);

	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer = (JSAMPROW) &buffer[cinfo.next_scanline
					  *(img->depth>>3)*img->width];
		jpeg_write_scanlines(&cinfo, &row_pointer, 1);
	}
	free(buffer);
	jpeg_finish_compress(&cinfo);
	fclose(outfile);

	return 0;
}



int write_jpeg(XImage *img, const char* filename)
{
	FILE* outfile;
	unsigned long pixel;
	int x, y;
	char* buffer;
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr       jerr;
	JSAMPROW row_pointer;

	outfile = fopen(filename, "wb");
	if (!outfile) {
		fprintf(stderr, "Failed to open output file %s", filename);
		return 1;
	}

	/* collect separate RGB values to a buffer */
	buffer = (char*)malloc(sizeof(char)*3*img->width*img->height);
	for (y = 0; y < img->height; y++) {
		for (x = 0; x < img->width; x++) {
			pixel = XGetPixel(img,x,y);
			buffer[y*img->width*3+x*3+0] = (char)(pixel>>16);
			buffer[y*img->width*3+x*3+1] = (char)((pixel&0x00ff00)>>8);
			buffer[y*img->width*3+x*3+2] = (char)(pixel&0x0000ff);
		}
	}

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, outfile);

	cinfo.image_width = img->width;
	cinfo.image_height = img->height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, 85, TRUE);
	jpeg_start_compress(&cinfo, TRUE);

	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer = (JSAMPROW) &buffer[cinfo.next_scanline
					  *(img->depth>>3)*img->width];
		jpeg_write_scanlines(&cinfo, &row_pointer, 1);
	}
	free(buffer);
	jpeg_finish_compress(&cinfo);
	fclose(outfile);

	return 0;
}



void print_image(XImage *img)
{
	char name[1024];
	sprintf(name, "bg%d.jpg", ++counter);
	write_jpeg(img, name);
}


void print_image2(XImage *img, std::vector<unsigned char> bg_red, std::vector<unsigned char> bg_green, std::vector<unsigned char> bg_blue)
{
	char name[1024];
	sprintf(name, "find%d.jpg", ++counter);
	write_jpeg2(img, name, bg_red, bg_green, bg_blue);
}
