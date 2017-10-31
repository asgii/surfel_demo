#version 430

layout (local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

/*
  Cover some number of adjacent pixels, by sampling.
  (Why adjacent? For locality of sampling - since these aren't
  interpolated samples, but just texels, ideally a bunch of texels
  will be lifted at once into the shared memory of the workgroup.
     
  A more precise scheme would be to check how swizzling works, if
  it does at all, for write-images. Actually, if there is none
  whatsoever and it's just a 1D buffer, then this locality thing
  actually hinders things. If this were Vulkan you could probably
  make sure...)

  How many pixels per? You want texels to be in shared memory so it
  depends on the size of the shared memory. Samples are each 4
  bytes. Shared memory might be 32kb for a thread group

  = 8,192 samples

  Still I'm not keen on just sqrt-ing it, because large groups will
  mean large redundant patches for remainders. Isn't it likely the
  card will stick more than one group in an SIMD anyway? Surely
  occupancy is about getting best fit. But fit in both the SIMD and
  the work...

  (NB the above is somewhat irrelevant given 8192 is well above limits
  for workgroup sizes)
*/

//Still includes depth at x, so it can be put in that way in
//previous stage; this stage must sort that out
layout (rgba8ui, binding = 1) readonly uniform uimage2D samples;

//The final image
layout (binding = 2) writeonly uniform uimage2D pixels;

layout (location = 5) uniform uvec2 pixelsXY;

uvec4 samp(ivec2 imageCoords)
{
   //Currently keeping it simple and just doing a pattern of samples,
   //without weighting based on subpixel location, etc.

   //Let's say for simplicity there are just 4x samples.
   //imageCoords are specifically coords for the final image, not the
   //samples buffer. Hence x2 below.

   uvec4 mix = uvec4(0.0);

   ivec2 baseCoords = imageCoords * 2;

   mix += imageLoad(samples, baseCoords) / 4;
   mix += imageLoad(samples, ivec2(baseCoords.x + 1, baseCoords.y)) / 4;
   mix += imageLoad(samples, baseCoords + 1) / 4;
   mix += imageLoad(samples, ivec2(baseCoords.x, baseCoords.y + 1)) / 4;

   //Swizzle because of zrgb in samples
//   return uvec4(mix.yzw, 1.0); //TODO currently I only really want
//   depth
   return uvec4(mix.x, mix.x, mix.x, 1.0);
}

void main()
{
   //In this shader each invocation should be assigned a specific
   //pixel (like a fragment shader with a fragment).
   const ivec2 coords = ivec2 (gl_GlobalInvocationID.xy);
   
   imageStore(pixels,
	      coords,
	      samp(coords));
}
