#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "lodepng.h"

void resize_and_greyscale(uint8_t* rgba, uint8_t* grey);
void zncc(uint8_t* dispmap, uint8_t* left, uint8_t* right, int width, int height, int disp_min, int disp_max);
void cross_check(uint8_t* dispmap_CC, uint8_t* dispmap_L, uint8_t* dispmap_R, int width, int height, int threshold);
void normalize(uint8_t* dispmap, unsigned width, unsigned height);

int main()
{
	// Error code for LodePNG
	uint32_t error;

	// Left and right RGBA images
	uint8_t* rgba_L;
	uint8_t* rgba_R;
	// Left and right greyscale images
	uint8_t* grey_L;
	uint8_t* grey_R;
	// Left and right disparity maps
	uint8_t* disp_LR;
	uint8_t* disp_RL;
	// Cross-checked disparity map
	uint8_t* disp_CC;

	// Width and height for input images
	unsigned w_in_L;
	unsigned h_in_L;
	unsigned w_in_R;
	unsigned h_in_R;
	// Width and height for output images
	unsigned w_out;
	unsigned h_out;


	// Decodes images
	// Left
	error = lodepng_decode32_file(&rgba_L, &w_in_L, &h_in_L, "im0.png");
	if (error) printf("Error %u: %s\n", error, lodepng_error_text(error));
	// Right
	error = lodepng_decode32_file(&rgba_R, &w_in_R, &h_in_R, "im1.png");
	if (error) printf("Error %u: %s\n", error, lodepng_error_text(error));


	// We check that images are of same size
	if ((w_in_L != w_in_R) || (h_in_L != h_in_R))
	{
		printf("Error: image sizes not matching");
		return 0;
	}


	// Resizes images
	w_out = w_in_L / 4;
	h_out = h_in_L / 4;
	// Left
	grey_L = (uint8_t*)malloc(w_out * h_out);
	resize_and_greyscale(rgba_L, grey_L);
	// Right
	grey_R = (uint8_t*)malloc(w_out * h_out);
	resize_and_greyscale(rgba_R, grey_R);


	// Calculates disparities
	// Left
	disp_LR = (uint8_t*)malloc(w_out * h_out);
	zncc(disp_LR, grey_L, grey_R, w_out, h_out, 0, 65);
	// Right
	disp_RL = (uint8_t*)malloc(w_out * h_out);
	zncc(disp_RL, grey_R, grey_L, w_out, h_out, -65, 0);


	// Cross-checking performed here
	//disp_CC = (uint8_t*)malloc(w_out * h_out);
	//cross_check(disp_CC, disp_LR, disp_RL, w_out, h_out, 8);


	// Normalizes image
	// Left
	normalize(disp_LR, w_out, h_out);
	// Right
	normalize(disp_RL, w_out, h_out);


	// Encodes the image.
	// Left
	error = lodepng_encode_file("dispL.png", disp_LR, w_out, h_out, LCT_GREY, 8);
	if (error) printf("Error %u: %s\n", error, lodepng_error_text(error));
	// Right
	error = lodepng_encode_file("dispR.png", disp_RL, w_out, h_out, LCT_GREY, 8);
	if (error) printf("Error %u: %s\n", error, lodepng_error_text(error));

	return 0;
}

void resize_and_greyscale(uint8_t* rgba, uint8_t* grey)
{
	int y_out, x_out, y_in, x_in;
	uint8_t red, green, blue;
	for (y_out = 0; y_out < 2016; y_out+=4)
	{
		for (x_out = 0; x_out < 2940; x_out+=4)
		{
			red = rgba[4 * 2940 * y_out + 4 * x_out];
			green = rgba[4 * 2940 * y_out + 4 * x_out + 1];
			blue = rgba[4 * 2940 * y_out + 4 * x_out + 2];
			y_in = y_out / 4;
			x_in = x_out / 4;
			grey[735 * y_in + x_in] = (uint8_t)(0.2126 * red + 0.7152 * green + 0.0722 * blue);
		}
	}
};

void zncc(uint8_t* dispmap, uint8_t* left, uint8_t* right, int width, int height, int disp_min, int disp_max)
{
	double score_best;
	double score_current;

	uint8_t disp_best;

	double lbmean;
	double rbmean;
	double lbstd;
	double rbstd;
	double cl;
	double cr;

	int index_L;
	int index_R;
	int win_width = 9;
	int win_height = 9;
	int win_size = win_width * win_height;
	int y_win;
	int x_win;
	int disp;

	int y_img;
	int x_img;

	for (y_img = 0; y_img < height; y_img++)
	{
		for (x_img = 0; x_img < width; x_img++)
		{
			disp_best = disp_max;
			score_best = -1;
			for (disp = disp_min; disp <= disp_max; disp++)
			{
				lbmean = 0;
				rbmean = 0;
				for (y_win = -win_height / 2; y_win < win_height / 2; y_win++)
				{
					for (x_win = -win_width / 2; x_win < win_width / 2; x_win++)
					{
						if (!(y_img + y_win >= 0) || !(y_img + y_win < height) || !(x_img + x_win >= 0) || !(x_img + x_win < width) || !(x_img + x_win - disp >= 0) || !(x_img + x_win - disp < width))
						{
							continue;
						}
						index_L = (y_img + y_win) * width + (x_img + x_win);
						index_R = (y_img + y_win) * width + (x_img + x_win - disp);

						lbmean += left[index_L];
						rbmean += right[index_R];
					}
				}
				lbmean /= win_size;
				rbmean /= win_size;

				lbstd = 0;
				rbstd = 0;

				score_current = 0;

				for (y_win = -win_height / 2; y_win < win_height / 2; y_win++)
				{
					for (x_win = -win_width / 2; x_win < win_width / 2; x_win++)
					{
						if (!(y_img + y_win >= 0) || !(y_img + y_win < height) || !(x_img + x_win >= 0) || !(x_img + x_win < width) || !(x_img + x_win - disp >= 0) || !(x_img + x_win - disp < width))
						{
							continue;
						}
						index_L = (y_img + y_win) * width + (x_img + x_win);
						index_R = (y_img + y_win) * width + (x_img + x_win - disp);

						cl = left[index_L] - lbmean;
						cr = right[index_R] - rbmean;

						lbstd += cl * cl;
						rbstd += cr * cr;

						score_current += cl * cr;
					}
				}
				score_current /= sqrt(lbstd) * sqrt(rbstd);

				if (score_current > score_best)
				{
					score_best = score_current;
					disp_best = disp;
				}
			}
			dispmap[735 * y_img + x_img] = disp_best;//(uint8_t)abs(disp_best);
		}
	}
};

void cross_check(uint8_t* dispmap_CC, uint8_t* dispmap_L, uint8_t* dispmap_R, int width, int height, int threshold)
{
	for (int i = 0; i < width * height; i++)
	{
		if (abs(dispmap_L[i] - dispmap_R[i]) > threshold)
		{
			dispmap_CC[i] = 0;
		}
		else
		{
			dispmap_CC[i] = dispmap_L[i];
		}
	}
};

void normalize(uint8_t* dispmap, unsigned width, unsigned height)
{
	unsigned max = 0;
	unsigned min = UCHAR_MAX;
	unsigned img_size = width * height;
	
	for (unsigned i = 0; i < img_size; i++)
	{
		if (dispmap[i] > max)
		{
			max = dispmap[i];
		}

		if (dispmap[i] < min)
		{
			min = dispmap[i];
		}
	}

	for (unsigned i = 0; i > img_size; i++)
	{
		dispmap[i] = (uint8_t)(255 * (dispmap[i] - min) / (max - min));
	}
};