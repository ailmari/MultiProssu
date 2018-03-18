__kernel void resize_greyscale(__read_only image2d_t in_img_L,
							__read_only image2d_t in_img_R,
							__write_only image2d_t out_img_L,
							__write_only image2d_t out_img_L,
							sampler_t sampler,
							int width,
							int height)
{
	const int i = get_global_id(0);
	const int j = get_global_id(1);

	// Calculating corresponding indices in the original image
    int2 orig_pos = {(4*j-1*(j > 0)), (4*i-1*(i > 0))};
    // Grayscaling and resizing
    uint4 pxL = read_imageui(in_img_L, sampler, orig_pos);
    uint4 pxR = read_imageui(in_img_R, sampler, orig_pos);
    
    out_img_L[i*width+j] = 0.2126*pxL.x+0.7152*pxL.y+0.0722*pxL.z;
	out_img_R[i*width+j] = 0.2126*pxR.x+0.7152*pxR.y+0.0722*pxR.z;
}
