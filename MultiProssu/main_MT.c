/*
#include <stdio.h>
#include <windows.h>
#include "lodepng.h"
#include <math.h>

#include <omp.h>

#define L_INPUT_IMAGE_NAME "im0.png"
#define R_INPUT_IMAGE_NAME "im1.png"

#define MAXDISP 65
#define MINDISP 0
#define WIN_SIZE_DISP 9

#define THRESHOLD 8

#define WIN_WIDTH_OC 32
#define WIN_HEIGHT_OC 32

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
	int error;

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
	int w_in_L;
	int h_in_L;
	int w_in_R;
	int h_in_R;
	// Width and height for output images
	int w_out;
	int h_out;


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


	// Disparities calculated here
	// Left-right pair
	printf("ZNCC for left-right...\n");
	disp_LR = (unsigned char*)malloc(w_out * h_out);
	zncc(disp_LR, grey_L, grey_R, w_out, h_out, MINDISP, MAXDISP);
	// Right-left pair
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


	// Normalization here
	printf("Normalization...\n");
	normalize(disp_CC_OF, w_out, h_out);


	// Stopping the timer
	QueryPerformanceCounter(&t2);
	elapsed_time = (t2.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart; // Time in milliseconds
	elapsed_time /= 1000.0; // Time in seconds
	printf("\nTotal execution time: %lf seconds\n\n", elapsed_time);


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
	error = lodepng_encode_file("output/disp_CC_OF_norm.png", disp_CC_OF, w_out, h_out, LCT_GREY, 8);
	if (error) printf("Error %u: %s\n", error, lodepng_error_text(error));

	return 0;
}

void resize_and_greyscale(unsigned char* rgba, unsigned char* grey)
{
	int y_out, x_out, y_in, x_in;
	unsigned char red, green, blue;
	//Height of Image
//#pragma omp parallel for private(y_out,x_out,y_in,x_in) shared(grey)
	for (y_out = 0; y_out < 2016; y_out += 4)
	{	//Width of Image
		for (x_out = 0; x_out < 2940; x_out += 4)
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
	double sum_of_window_values1;
	double sum_of_window_values2;
	double standart_deviation1;
	double standart_deviation2;
	double multistand = 0;
	double best_disparity_value;
	double current_max_sum;
	double average1, average2;
	int d;
	int WIN_Y, WIN_X;
	double var1, var2;
	int y, x;
#pragma omp parallel for private(y,x,sum_of_window_values1, sum_of_window_values2, standart_deviation1, standart_deviation2, multistand, best_disparity_value, current_max_sum, average1, average2, d, WIN_Y, WIN_X, var1, var2) shared(dmap)
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			best_disparity_value = max_disp;
			//Init for next value
			current_max_sum = 0;
			for (d = min_disp; d <= max_disp; d++)
			{
				sum_of_window_values1 = 0;
				sum_of_window_values2 = 0;
				for (WIN_Y = -WIN_SIZE_DISP / 2; WIN_Y < WIN_SIZE_DISP / 2; WIN_Y++)
				{
					for (WIN_X = -WIN_SIZE_DISP / 2; WIN_X < WIN_SIZE_DISP / 2; WIN_X++)
					{
						if (y + WIN_Y >= 0 &&
							x + WIN_X >= 0 &&
							y + WIN_Y < height &&
							x + WIN_X < width &&
							x + WIN_X - d >= 0 &&
							x + WIN_X - d < width)
						{
							//Calculate the mean value for each window
							sum_of_window_values1 += image1[(WIN_Y + y)*width + (x + WIN_X)];
							sum_of_window_values2 += image2[(WIN_Y + y)*width + (x + WIN_X - d)];
						}
						else
						{
							continue;
						}
					}
				}
				average1 = sum_of_window_values1 / (WIN_SIZE_DISP*WIN_SIZE_DISP);
				average2 = sum_of_window_values2 / (WIN_SIZE_DISP*WIN_SIZE_DISP);
				var1 = 0;
				var2 = 0;
				multistand = 0;
				//window_sum = 0;
				for (WIN_Y = -WIN_SIZE_DISP / 2; WIN_Y < WIN_SIZE_DISP / 2; WIN_Y++)
				{
					for (WIN_X = -WIN_SIZE_DISP / 2; WIN_X < WIN_SIZE_DISP / 2; WIN_X++)
					{
						if (y + WIN_Y >= 0 &&
							x + WIN_X >= 0 &&
							y + WIN_Y < height &&
							x + WIN_X < width &&
							x + WIN_X - d >= 0 &&
							x + WIN_X - d < width)
						{
							//Calculate the actual ZNCC value for each windows
							standart_deviation1 = image1[(WIN_Y + y)*width + (x + WIN_X)] - average1;
							standart_deviation2 = image2[(WIN_Y + y)*width + (x + WIN_X - d)] - average2;
							//Variance
							var1 += standart_deviation1*standart_deviation1;
							var2 += standart_deviation2*standart_deviation2;
							multistand += standart_deviation1*standart_deviation2;
						}
						else
						{
							continue;
						}
					}
				}
				multistand /= sqrt(var1) * sqrt(var2);
				if (multistand > current_max_sum)
				{
					current_max_sum = multistand;
					best_disparity_value = d;
				}

			}
			dmap[y * width + x] = (unsigned char)abs(best_disparity_value);
		}
	}
}

void cross_check(unsigned char* dispmap_CC, unsigned char* dispmap_L, unsigned char* dispmap_R, int width, int height, int threshold)
{
	int i;
//#pragma omp parallel for private(i)
	for (i = 0; i < width * height; i++)
	{	//If absolute value of L and R depthmaps differ by treshold
		//Set disparity to 0
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
	int win_width = WIN_WIDTH_OC;
	int win_height = WIN_HEIGHT_OC;
	int sum_win;
	int pixel_count;
	int mean_win;
	int y, x;
#pragma omp parallel for private(y,x,sum_win,pixel_count,mean_win)
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			if (dispmap_CC[y * width + x] == 0)
			{
				sum_win = 0;
				pixel_count = 0;

				for (int y_win = -win_height; y_win <= win_height; y_win++)
				{
					for (int x_win = -win_width; x_win <= win_width; x_win++)
					{
						if (!(y + y_win >= 0) ||
							!(y + y_win < height) ||
							!(x + x_win >= 0) ||
							!(x + x_win < width))
						{
							continue;
						}
						if (dispmap_CC[y * width + y_win + x + x_win] != 0)
						{
							sum_win += dispmap_CC[y * width + y_win + x + x_win];
							pixel_count++;
						}
					}
				}
				if (pixel_count > 0)
				{
					mean_win = sum_win / pixel_count;
					dispmap_OF[y * width + x] = mean_win;
				}
			}
			else
			{
				dispmap_OF[y * width + x] = dispmap_CC[y * width + x];
			}
		}
	}
};

void normalize(unsigned char* dispmap, int width, int height)
{
	//Nomalization, upper limit is allways MAX Disparity
	int max = MAXDISP;
	int min = 0;

	for (int i = 0; i < width * height; i++)
	{
		dispmap[i] = (unsigned char)(255 * (dispmap[i] - min) / (max - min));
	}
};
*/