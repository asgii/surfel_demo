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
*/

//Still includes depth at x, so it can be put in that way in
//previous stage; this stage must sort that out
layout (rgba8ui, binding = 1) readonly uniform uimage2D samples;
layout (location = 3) uniform uint samplesX;
layout (location = 4) uniform uint samplesY;

//The final image
layout (binding = 2) writeonly uniform uimage2D pixels; //TODO binding
layout (location = 5) uniform uint pixelsX;
layout (location = 6) uniform uint pixelsY;

uvec4 samp(ivec2 imageCoords)
{
   //Currently keeping it simple and just doing a pattern of samples,
   //without weighting based on subpixel location, etc.

   //Let's say for simplicity there are just 4x samples.
   //imageCoords are specifically coords for the final image, not the
   //samples buffer.

   uvec4 mix = uvec4(0.0);

   for (uint i = 0; i < 4; ++i)
   {
      mix += imageLoad(samples, ivec2(imageCoords.x * 2 + i / 2,
				      imageCoords.y * 2 + mod(i, 2))) / 4;
   }

   //Swizzle because of zrgb in samples
   return uvec4(mix.xyz, 0.0);
}

void main()
{
   const ivec2 coords = ivec2 (gl_GlobalInvocationID.xy);

   imageStore(pixels,
	      coords,
	      uvec4(samp(coords).gba, 1)); //r is depth in samples.
}
