#include <stdio.h>
#include <windows.h>
#include <math.h>

#include "lodepng.h"
#include "CL\cl.h"

#define L_INPUT_IMAGE_NAME "im0.png"
#define R_INPUT_IMAGE_NAME "im1.png"

cl_kernel clOneKernelPlease(cl_context context, cl_device_id device_id, const char* file_name, const char* kernel_name);
void clPrintInfo(cl_platform_id platform_id, cl_device_id device_id);
int clCheckStatus(cl_int status_code);

int main()
{
	cl_int status;

	// Gets platform and device
	cl_platform_id platform_id;
	cl_device_id device_id;
	cl_uint platform_count;
	cl_uint device_count;

	clGetPlatformIDs(1, &platform_id, &platform_count);
	clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, &device_count);

	clPrintInfo(platform_id, device_id);


	// Creates context
	printf("\nCreating context...\n");
	cl_context context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &status);
	clCheckStatus(status);


	// Creates command queue
	printf("Creating command queue...\n");
	cl_command_queue command_queue = clCreateCommandQueue(context, device_id, 0, &status);
	clCheckStatus(status);


	// Decodes images
	printf("Decoding images...\n");
	unsigned char* rgba_L;
	unsigned char* rgba_R;

	int w_in;
	int h_in;

	lodepng_decode32_file(&rgba_L, &w_in, &h_in, L_INPUT_IMAGE_NAME);
	lodepng_decode32_file(&rgba_R, &w_in, &h_in, R_INPUT_IMAGE_NAME);

	int w_out = w_in / 4;
	int h_out = h_in / 4;


	// Creates OpenCL image objects
	printf("Creating image objects...\n");
	cl_image_format image_format;
	image_format.image_channel_order = CL_RGBA;
	image_format.image_channel_data_type = CL_UNSIGNED_INT8;

	cl_image_desc image_desc;
	image_desc.image_type = CL_MEM_OBJECT_IMAGE2D;
	image_desc.image_width = w_in;
	image_desc.image_height = h_in;
	image_desc.image_depth = 8;
	image_desc.image_row_pitch = w_in * 4;
	image_desc.image_slice_pitch = 0;
	image_desc.num_mip_levels = 0;
	image_desc.num_samples = 0;
	image_desc.buffer = NULL;

	cl_mem img_L = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, &image_format, &image_desc, rgba_L, &status);
	clCheckStatus(status);
	cl_mem img_R = clCreateImage(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, &image_format, &image_desc, rgba_R, &status);
	clCheckStatus(status);


	// Buffers for images
	printf("Creating image buffers...\n");
	cl_mem buff_L = clCreateBuffer(context, CL_MEM_READ_WRITE, w_out*h_out, 0, &status);
	clCheckStatus(status);
	cl_mem buff_R = clCreateBuffer(context, CL_MEM_READ_WRITE, w_out*h_out, 0, &status);
	clCheckStatus(status);
	cl_mem buff_disp_LR = clCreateBuffer(context, CL_MEM_READ_WRITE, w_out*h_out, 0, &status);
	clCheckStatus(status);


	// Creates sampler
	printf("Creating sampler...\n");
	cl_sampler sampler = clCreateSampler(context, CL_FALSE, CL_ADDRESS_CLAMP_TO_EDGE, CL_FILTER_NEAREST, &status);
	clCheckStatus(status);


	// Create kernels
	cl_kernel resize_greyscale_kernel = clOneKernelPlease(context, device_id, "resize_greyscale.cl", "resize_greyscale");
	cl_kernel zncc_kernel = clOneKernelPlease(context, device_id, "zncc.cl", "zncc");


	// Set kernel arguments
	clSetKernelArg(resize_greyscale_kernel, 0, sizeof(cl_mem), &img_L);
	clSetKernelArg(resize_greyscale_kernel, 1, sizeof(cl_mem), &img_R);
	clSetKernelArg(resize_greyscale_kernel, 2, sizeof(cl_mem), &buff_L);
	clSetKernelArg(resize_greyscale_kernel, 3, sizeof(cl_mem), &buff_R);
	clSetKernelArg(resize_greyscale_kernel, 4, sizeof(cl_sampler), &sampler);
	clSetKernelArg(resize_greyscale_kernel, 5, sizeof(cl_int), &w_out);
	clSetKernelArg(resize_greyscale_kernel, 6, sizeof(cl_int), &h_out);


	// Execute!
	printf("Executing kernel...\n");
	size_t localWorkSize[2] = { 35, 24 };
	size_t globalWorkSize[2] = { w_out, h_out };

	status = clEnqueueNDRangeKernel(command_queue, resize_greyscale_kernel, 2, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
	clCheckStatus(status);
	clFinish(command_queue);


	// Read buffer for results
	unsigned char* disp = (unsigned char*)malloc(w_out * h_out);
	size_t region[3] = { w_out, h_out, 0 };
	status = clEnqueueReadBuffer(command_queue, buff_L, CL_TRUE, 0, w_out*h_out, disp, 0, NULL, NULL);
	clCheckStatus(status);


	// Encode and save results
	lodepng_encode_file("output/test.png", disp, w_out, h_out, LCT_GREY, 8);

	return 0;
}

cl_kernel clOneKernelPlease(cl_context context, cl_device_id device_id, const char* file_name, const char* kernel_name)
{
	//	This function simplifies creation of multiple kernels,
	//	It wraps filereading, program building and kernel creation.

	cl_int status;

	// Load the kernel source code into source_str
	FILE *fp;
	char *source_str;
	size_t source_size;

	fopen_s(&fp, file_name, "r");
	source_str = (char*)malloc(0x100000);
	source_size = fread(source_str, 1, 0x100000, fp);
	fclose(fp);

	// Create and build a program
	cl_program program = clCreateProgramWithSource(context, 1, (const char**)&source_str, (const size_t *)&source_size, &status);
	clCheckStatus(status);

	status = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (status != CL_SUCCESS)
	{
		printf("clBuildProgram error... %d\n", status);

		if (status != CL_SUCCESS) {
			char *buff_erro;
			cl_int errcode;
			size_t build_log_len;
			errcode = clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &build_log_len);
			if (errcode) {
				printf("clGetProgramBuildInfo failed at line %d\n", __LINE__);
				exit(-1);
			}

			buff_erro = malloc(build_log_len);
			if (!buff_erro) {
				printf("malloc failed at line %d\n", __LINE__);
				exit(-2);
			}

			errcode = clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, build_log_len, buff_erro, NULL);
			if (errcode) {
				printf("clGetProgramBuildInfo failed at line %d\n", __LINE__);
				exit(-3);
			}

			fprintf(stderr, "Build log: \n%s\n", buff_erro); //Be careful with  the fprint
			free(buff_erro);
			fprintf(stderr, "clBuildProgram failed\n");
			exit(EXIT_FAILURE);
		}
	}

	// Create kernel
	cl_kernel kernel = clCreateKernel(program, kernel_name, &status);
	clCheckStatus(status);

	return kernel;
}

void clPrintInfo(cl_platform_id platform_id, cl_device_id device_id)
{
	cl_char string[10240] = { 0 };
	cl_uint num;
	cl_ulong mem_size;
	cl_device_local_mem_type mem_type;
	size_t size;
	size_t dims[3];

	printf("------ PLATFORM ------\n");

	clGetPlatformInfo(platform_id, CL_PLATFORM_NAME, sizeof(string), &string, NULL);
	printf("Name:			%s\n", string);

	clGetPlatformInfo(platform_id, CL_PLATFORM_VERSION, sizeof(string), &string, NULL);
	printf("Version:		%s\n", string);

	printf("\n------- DEVICE -------\n");

	clGetDeviceInfo(device_id, CL_DEVICE_NAME, sizeof(string), &string, NULL);
	printf("Name:			%s\n", string);

	clGetDeviceInfo(device_id, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &num, NULL);
	printf("Max compute units:	%d\n", num);

	clGetDeviceInfo(device_id, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &mem_size, NULL);
	printf("Local memory size:	%llu KB\n", mem_size/1024);

	clGetDeviceInfo(device_id, CL_DEVICE_LOCAL_MEM_TYPE, sizeof(cl_device_local_mem_type), &mem_type, NULL);
	printf("Local memory type:	0x%u\n", mem_type);

	clGetDeviceInfo(device_id, CL_DEVICE_MAX_CLOCK_FREQUENCY , sizeof(cl_uint), &num, NULL);
	printf("Max clock frequency:	%d MHz\n", num);

	clGetDeviceInfo(device_id, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, sizeof(cl_ulong), &mem_size, NULL);
	printf("Max buffer size:	%llu KB\n", mem_size/1024);

	clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &size, NULL);
	printf("Max work group size:	%zu B\n", size);

	clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(dims), &dims, NULL);
	printf("Max work item sizes:	%ld B * %ld B * %ld B\n", dims[0], dims[1], dims[2]);
}

int clCheckStatus(cl_int status_code)
{
	if (status_code != CL_SUCCESS)
	{
		printf("OpenCL error... %d\n", status_code);
		return 1;
	}
}
