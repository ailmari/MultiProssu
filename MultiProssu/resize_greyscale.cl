__kernel void resize_greyscale(__read_only image2d_t in_img_L,
							__read_only image2d_t in_img_R,
							__global uchar *out_img_L,
							__global uchar *out_img_R,
							sampler_t sampler,
							int width,
							int height)
{
	const int x = get_global_id(0);
	const int y = get_global_id(1);
    
	// Sample original image
	int2 coords = {4*x, 4*y};
    uint4 pxl_L = read_imageui(in_img_L, sampler, coords);
    uint4 pxl_R = read_imageui(in_img_R, sampler, coords);
    
	// Write resized and greyscale image to buffer
    out_img_L[y*width + x] = 0.2126*pxl_L.x + 0.7152*pxl_L.y + 0.0722*pxl_L.z;
	out_img_R[y*width + x] = 0.2126*pxl_R.x + 0.7152*pxl_R.y + 0.0722*pxl_R.z;
}
