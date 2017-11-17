#version 430

layout (local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

//layout (rgba8ui, binding = 1) readonly uniform uimage2D samples;
layout (r32ui, binding = 1) readonly uniform uimage2D samples;

//The final image
layout (binding = 2) writeonly uniform uimage2D pixels;

layout (location = 5) uniform uvec2 pixelsXY;

uvec4 samp(ivec2 imageCoords)
{
   /*
     Currently keeping it simple and just doing a pattern of samples,
     without weighting based on subpixel location, etc.

     Let's say for simplicity there are just 4x samples.
     imageCoords are specifically coords for the final image, not the
     samples buffer. Hence x2 below.
   */

   uvec4 mix = uvec4(0.0);

   ivec2 baseCoords = imageCoords * 2;

   mix += imageLoad(samples, baseCoords) / 4;
   mix += imageLoad(samples, ivec2(baseCoords.x + 1, baseCoords.y)) / 4;
   mix += imageLoad(samples, baseCoords + 1) / 4;
   mix += imageLoad(samples, ivec2(baseCoords.x, baseCoords.y + 1)) / 4;

   return mix;
}

void main()
{
   //In this shader each invocation should be assigned a specific
   //pixel (like a fragment shader with a fragment).
   const ivec2 coords = ivec2 (gl_GlobalInvocationID.xy);

   //imageLoad can only return uvec4, so have to reconstruct single
   //value.
   //(Just storing it direct won't work- lower-significance colours
   //will loop from 0 to 1 as the depth increases.)
   //Since they've always been uints, the change in size from 8bit to
   //32bit should simply have increased # leading 0s by 24.
   uvec4 colour = samp(coords);

   //TODO What about endianness? (Undefined in GLSL spec afaik)
//   uint recValue = ((colour.r << 24) |
//		    (colour.g << 16) |
//		    (colour.b << 8)  |
//		    colour.a);

   //Scale to 8bit (255)
   const float scale = 255.0 / float(~uint(0));

//   float scaledValue = float(recValue) * scale;

   uint finalValue = uint(float(colour.r) * scale);
   
   imageStore(pixels,
	      coords,
	      uvec4(finalValue, finalValue, finalValue, 255));
}
