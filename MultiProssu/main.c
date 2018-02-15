#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <windows.h>
#include "lodepng.h"

#define L_INPUT_IMAGE_NAME "im0.png"
#define R_INPUT_IMAGE_NAME "im1.png"

#define MAXDISP 65
#define MINDISP 0
#define WIN_SIZE 9

#define THRESHOLD 8

void resize_and_greyscale(unsigned char* rgba, unsigned char* grey);
void zncc(unsigned char* dmap, unsigned char* image1, unsigned char* image2, int width, int height, int min_disp, int max_disp);
void cross_check(unsigned char* dispmap_CC, unsigned char* dispmap_L, unsigned char* dispmap_R, int width, int height, int threshold);
void occlusion_fill(unsigned char* dispmap_OF, unsigned char* dispmap_CC, int width, int height);
void normalize(unsigned char* dispmap, int width, int height);

int main()
{
	// Timer
	LARGE_INTEGER frequency;
	LARGE_INTEGER t1;
	LARGE_INTEGER t2;
	double elapsed_time;

	// Error code for LodePNG
	uint32_t error;

	// Left and right RGBA images
	unsigned char* rgba_L;
	unsigned char* rgba_R;
	// Left and right greyscale images
	unsigned char* grey_L;
	unsigned char* grey_R;
	// Left and right disparity maps
	unsigned char* disp_LR;
	unsigned char* disp_RL;
	// Cross-checked disparity map
	unsigned char* disp_CC;
	// Cross-checked and occlusion-filled disparity map
	unsigned char* disp_CC_OF;

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
	error = lodepng_decode32_file(&rgba_L, &w_in_L, &h_in_L, L_INPUT_IMAGE_NAME);
	if (error) printf("Error %u: %s\n", error, lodepng_error_text(error));
	// Right
	error = lodepng_decode32_file(&rgba_R, &w_in_R, &h_in_R, R_INPUT_IMAGE_NAME);
	if (error) printf("Error %u: %s\n", error, lodepng_error_text(error));


	// Starting the timer
	QueryPerformanceFrequency(&frequency); // Get ticks per second
	QueryPerformanceCounter(&t1);


	// Resizes images
	w_out = w_in_L / 4;
	h_out = h_in_L / 4;
	// Left
	printf("Resize and greyscale for left...\n");
	grey_L = (unsigned char*)malloc(w_out * h_out);
	resize_and_greyscale(rgba_L, grey_L);
	// Right
	printf("Resize and greyscale for right...\n");
	grey_R = (unsigned char*)malloc(w_out * h_out);
	resize_and_greyscale(rgba_R, grey_R);

	
	// Calculates disparities
	// Left
	printf("ZNCC for left-right...\n");
	disp_LR = (unsigned char*)malloc(w_out * h_out);
	zncc(disp_LR, grey_L, grey_R, w_out, h_out, MINDISP, MAXDISP);
	// Right
	printf("ZNCC for right-left...\n");
	disp_RL = (unsigned char*)malloc(w_out * h_out);
	zncc(disp_RL, grey_R, grey_L, w_out, h_out, -MAXDISP, MINDISP);

	
	// Cross-checking performed here
	printf("Cross-checking...\n");
	disp_CC = (unsigned char*)malloc(w_out * h_out);
	cross_check(disp_CC, disp_LR, disp_RL, w_out, h_out, THRESHOLD);

	
	// Occlusion filling here
	printf("Occlusion-filling...\n");
	disp_CC_OF = (unsigned char*)malloc(w_out * h_out);
	occlusion_fill(disp_CC_OF, disp_CC, w_out, h_out);


	// Normalizes image
	printf("Normalization...\n");
	normalize(disp_CC_OF, w_out, h_out);
	

	// Stopping the timer
	QueryPerformanceCounter(&t2);
	elapsed_time = (t2.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart; // Time in ms
	elapsed_time /= 1000.0;
	printf("\nTotal execution time: %lf\n\n", elapsed_time);
	

	// Encodes the images
	// Grey left
	error = lodepng_encode_file("output/grey_L.png", grey_L, w_out, h_out, LCT_GREY, 8);
	if (error) printf("Error %u: %s\n", error, lodepng_error_text(error));
	// Grey right
	error = lodepng_encode_file("output/grey_R.png", grey_R, w_out, h_out, LCT_GREY, 8);
	if (error) printf("Error %u: %s\n", error, lodepng_error_text(error));
	// Disparity left-right
	error = lodepng_encode_file("output/disp_LR.png", disp_LR, w_out, h_out, LCT_GREY, 8);
	if (error) printf("Error %u: %s\n", error, lodepng_error_text(error));
	// Disparity right-left
	error = lodepng_encode_file("output/disp_RL.png", disp_RL, w_out, h_out, LCT_GREY, 8);
	if (error) printf("Error %u: %s\n", error, lodepng_error_text(error));
	// Cross-checked
	error = lodepng_encode_file("output/disp_CC.png", disp_CC, w_out, h_out, LCT_GREY, 8);
	if (error) printf("Error %u: %s\n", error, lodepng_error_text(error));
	// Cross-checked & occlusion-filled & normalized
	error = lodepng_encode_file("output/disp_CC_OF.png", disp_CC_OF, w_out, h_out, LCT_GREY, 8);
	if (error) printf("Error %u: %s\n", error, lodepng_error_text(error));

	return 0;
}

void resize_and_greyscale(unsigned char* rgba, unsigned char* grey)
{
	int y_out, x_out, y_in, x_in;
	unsigned char red, green, blue;
	for (y_out = 0; y_out < 2016; y_out+=4)
	{
		for (x_out = 0; x_out < 2940; x_out+=4)
		{
			red = rgba[4 * 2940 * y_out + 4 * x_out];
			green = rgba[4 * 2940 * y_out + 4 * x_out + 1];
			blue = rgba[4 * 2940 * y_out + 4 * x_out + 2];
			y_in = y_out / 4;
			x_in = x_out / 4;
			grey[735 * y_in + x_in] = (unsigned char)(0.2126 * red + 0.7152 * green + 0.0722 * blue);
		}
	}
};

void zncc(unsigned char* dmap, unsigned char* image1, unsigned char* image2, int width, int height, int min_disp, int max_disp)
{
	//int Standard_deviation;
	double sum_of_window_values1;
	double sum_of_window_values2;
	double standart_deviation1;
	double standart_deviation2;
	double multistand = 0;
	double best_disparity_value;
	double current_max_sum;
	double average1, average2;
	int J;
	int I;
	int d;
	int WIN_Y, WIN_X;
	double std1, std2;

	for (J = 0; J < height; J++) {
		for (I = 0; I < width; I++) {
			best_disparity_value = max_disp;
			current_max_sum = -1;
			for (d = min_disp; d <= max_disp; d++) {
				sum_of_window_values1 = 0;
				sum_of_window_values2 = 0;
				for (WIN_Y = -WIN_SIZE / 2; WIN_Y < WIN_SIZE / 2; WIN_Y++) {
					for (WIN_X = -WIN_SIZE / 2; WIN_X < WIN_SIZE / 2; WIN_X++) {

						if (J + WIN_Y >= 0 &&
							I + WIN_X >= 0 &&
							J + WIN_Y < height &&
							I + WIN_X < width &&
							I + WIN_X - d >= 0 &&
							I + WIN_X - d < width)
						{
							//Calculate the mean value for each window
							sum_of_window_values1 += image1[(WIN_Y + J)*width + (I + WIN_X)];
							sum_of_window_values2 += image2[(WIN_Y + J)*width + (I + WIN_X - d)];
						}
						else
							continue;


					}
				}
				average1 = sum_of_window_values1 / (WIN_SIZE*WIN_SIZE);
				average2 = sum_of_window_values2 / (WIN_SIZE*WIN_SIZE);
				std1 = 0;
				std2 = 0;
				multistand = 0;
				//window_sum = 0;
				for (WIN_Y = -WIN_SIZE / 2; WIN_Y < WIN_SIZE / 2; WIN_Y++) {
					for (WIN_X = -WIN_SIZE / 2; WIN_X < WIN_SIZE / 2; WIN_X++) {

						if (J + WIN_Y >= 0 &&
							I + WIN_X >= 0 &&
							J + WIN_Y < height &&
							I + WIN_X < width &&
							I + WIN_X - d >= 0 &&
							I + WIN_X - d < width)
						{
							//Calculate the actual ZNCC value for each windows
							standart_deviation1 = image1[(WIN_Y + J)*width + (I + WIN_X)] - average1;
							standart_deviation2 = image2[(WIN_Y + J)*width + (I + WIN_X - d)] - average2;
							std1 += standart_deviation1*standart_deviation1;
							std2 += standart_deviation2*standart_deviation2;
							multistand += standart_deviation1*standart_deviation2;
						}
						else
							continue;

					}
				}
				multistand /= sqrt(std1) * sqrt(std2);
				if (multistand > current_max_sum) {
					current_max_sum = multistand;
					best_disparity_value = d;
				}

			}
			dmap[width*J + I] = (unsigned char)abs(best_disparity_value);
		}
	}
}

void cross_check(unsigned char* dispmap_CC, unsigned char* dispmap_L, unsigned char* dispmap_R, int width, int height, int threshold)
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

void occlusion_fill(unsigned char* dispmap_OF, unsigned char* dispmap_CC, int width, int height)
{
	int win_width = 2;
	int win_height = 2;
	int sum_win;
	int pixel_count;
	int mean_win;

	for (int y_img = 0; y_img < height; y_img++)
	{
		for (int x_img = 0; x_img < width; x_img++)
		{
			if (dispmap_CC[y_img * width + x_img] == 0)
			{
				sum_win = 0;
				pixel_count = 0;

				for (int y_win = -win_height; y_win <= win_height; y_win++)
				{
					for (int x_win = -win_width; x_win <= win_width; x_win++)
					{
						if (!(y_img + y_win >= 0) ||
							!(y_img + y_win < height) ||
							!(x_img + x_win >= 0) ||
							!(x_img + x_win < width))
						{
							continue;
						}
						if (dispmap_CC[y_img * width + y_win + x_img + x_win] != 0)
						{
							sum_win += dispmap_CC[y_img * width + y_win + x_img + x_win];
							pixel_count++;
						}
					}
				}
				if (pixel_count > 0)
				{
					mean_win = sum_win / pixel_count;
					dispmap_OF[y_img * width + x_img] = mean_win;
				}
			}
			else
			{
				dispmap_OF[y_img * width + x_img] = dispmap_CC[y_img * width + x_img];
			}
		}
	}
};

void normalize(unsigned char* dispmap, int width, int height)
{
	int max = 65;
	int min = 0;

	for (int i = 0; i < width * height; i++)
	{
		dispmap[i] = (unsigned char)(255 * (dispmap[i] - min) / (max - min));
	}
};