#version 430

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

//1D on the basis that the buffer is 1D

/*
  Doesn't make much sense to order them around the number of samples,
  because a surfel model might not take up much (or any) of the
  samples. Instead, like a vert shader, must be ordered around the
  number of surfels. The surfels are just a 1D buffer.
*/

layout (std430, binding = 1) buffer surfelsBlock
{
   vec4 data[];
} surfels;

//extra qualifier, 'shared' (can't initialise)

uniform mat4 perspective;

layout (r32ui, binding = 7) uniform uimage2D samples; //Output

layout (location = 3) uniform uint samplesX;
layout (location = 4) uniform uint samplesY;

ivec2 getScreenCoords(const vec4 pt)
{
   //Should be in clip-space already

   //TODO might have to unnormalise or similar

   return ivec2(pt.xy / imageSize(samples));
}

void main()
{
   vec4 data = surfels.data[gl_GlobalInvocationID.x];

   //Transform point
   //TODO how do out-of-bounds accesses work w/r/t local size?
   vec4 point = vec4(data.xyz, 1.0) * perspective;
   
   uint z = uint(point.z / 4) << 24; //Extremely TODO
   uint r = 1 >> 8;

   uint value = z | r;
   
   //Depth followed by rgb
   imageAtomicMax(samples,
		  getScreenCoords(point),
		  value);
}
