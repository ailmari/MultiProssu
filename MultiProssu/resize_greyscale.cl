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

	// Calculating corresponding indices in the original image
    int2 orig_pos = {x, y};
    // Grayscaling and resizing
    uint4 pxL = read_imageui(in_img_L, sampler, orig_pos);
    uint4 pxR = read_imageui(in_img_R, sampler, orig_pos);
    
    out_img_L[y*width+x] = 0.2126*pxL.x+0.7152*pxL.y+0.0722*pxL.z;
	out_img_R[y*width+x] = 0.2126*pxR.x+0.7152*pxR.y+0.0722*pxR.z;
}
