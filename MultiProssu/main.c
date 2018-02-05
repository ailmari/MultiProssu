#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "lodepng.h"

void resize_and_greyscale(uint8_t* image_in, uint8_t* image_out);

int main()
{
	uint32_t error;

	uint8_t* orig_image_left;
	uint8_t* orig_image_right;
	uint8_t* image_left;
	uint8_t* image_right;

	uint32_t width_in_left, height_in_left;
	uint32_t width_in_right, height_in_right;
	uint32_t width_out, height_out;

	// Decodes images.
	error = lodepng_decode32_file(&orig_image_left, &width_in_left, &height_in_left, "im0.png");
	if (error) printf("Error %u: %s\n", error, lodepng_error_text(error));

	error = lodepng_decode32_file(&orig_image_right, &width_in_right, &height_in_right, "im1.png");
	if (error) printf("Error %u: %s\n", error, lodepng_error_text(error));

	// We check that images are of same size.
	if ((width_in_left != width_in_right) || (height_in_left != height_in_right))
	{
		printf("Error: image sizes not matching");
		return 0;
	}

	// Resizes images.
	width_out = width_in_left / 4;
	height_out = height_in_left / 4;

	image_left = (uint8_t*) malloc(width_out * height_out);
	resize_and_greyscale(orig_image_left, image_left);
	image_right = (uint8_t*)malloc(width_out * height_out);
	resize_and_greyscale(orig_image_right, image_right);

	// Encodes the image.
	error = lodepng_encode_file("im0_greyscale.png", image_left, width_out, height_out, LCT_GREY, 8);
	if (error) printf("Error %u: %s\n", error, lodepng_error_text(error));

	return 0;
}

void resize_and_greyscale(uint8_t* image_in, uint8_t* image_out)
{
	uint32_t y_out, x_out, y_in, x_in;
	uint8_t red, green, blue;
	for (y_out = 0; y_out < 2016; y_out+=4)
	{
		for (x_out = 0; x_out < 2940; x_out+=4)
		{
			red = image_in[4 * 2940 * y_out + 4 * x_out];
			green = image_in[4 * 2940 * y_out + 4 * x_out + 1];
			blue = image_in[4 * 2940 * y_out + 4 * x_out + 2];
			y_in = y_out / 4;
			x_in = x_out / 4;
			image_out[735 * y_in + x_in] = 0.2126 * red + 0.7152 * green + 0.0722 * blue;
		}
	}
};