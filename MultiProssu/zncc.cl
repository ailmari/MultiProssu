__kernel void zncc(__global uchar *in_img_L,
				__global uchar *in_img_R,
				__global uchar *dmap,
				sampler_t sampler,
				int width,
				int height)
{
	const int x = get_global_id(0);
	const int y = get_global_id(1);
}
