#include <stdio.h>
#include <windows.h>
#include <math.h>

#include "lodepng.h"
#include "CL\cl.h"

void printInfo(cl_platform_id platform_id, cl_device_id device_id);

int main()
{
	cl_platform_id platform_id;
	cl_device_id device_id;
	cl_uint platform_count;
	cl_uint device_count;

	// Get platform and device
	clGetPlatformIDs(1, &platform_id, &platform_count);
	clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, &device_count);

	printInfo(platform_id, device_id);

	return 0;
}

void printInfo(cl_platform_id platform_id, cl_device_id device_id)
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
