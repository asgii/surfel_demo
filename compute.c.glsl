#version 430

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

//1D on the basis that the buffer is 1D

/*
  Doesn't make much sense to order them around the number of samples,
  because a surfel model might not take up much (or any) of the
  samples. Instead, like a vert shader, must be ordered around the
  number of surfels. The surfels are just a 1D buffer.
*/

layout (std430, binding = 3) buffer surfelsBlock
{
   vec4 data[];
} surfels;

uniform mat4 perspective;

layout (r32ui, binding = 1) uniform uimage2D samples;

//TODO why not make this a uvec? One less glUniform, too
layout (location = 3) uniform uvec2 samplesXY;

layout (location = 5) uniform uvec2 pixelsXY;

ivec2 getScreenCoords(ivec2 ptxy)
{
   ivec2 dimensions = ivec2(samplesXY);

   ivec2 coords = (ptxy * dimensions + dimensions) / 2; // / 2;

   if ((coords.x > samplesXY.x) || (coords.x < 0) ||
       (coords.y > samplesXY.y) || (coords.y < 0))
   {
      return ivec2(-1, -1);
   }

   else return coords;
}

void main()
{
   //The start of this workgroup.
   uint baseIndex = gl_WorkGroupID.z * gl_WorkGroupSize.z * gl_WorkGroupSize.x * gl_WorkGroupSize.y +
      gl_WorkGroupID.y * gl_WorkGroupSize.y * gl_WorkGroupSize.x +
      gl_WorkGroupID.x * gl_WorkGroupSize.x;

   //gl_LocalInvocationIndex is actually like this:
/*   uint baseIndex = gl_WorkGroupID.z * gl_WorkGroupSize.x * gl_WorkGroupSize.y +
      gl_WorkGroupID.y * gl_WorkGroupSize.x +
      gl_WorkGroupID.x;*/
   
   uint globalIndex = baseIndex + gl_LocalInvocationIndex;
   //gl_LocalInvocationIndex isn't right because it's local. There is
   //no GlobalInvocationIndex.

   //Assuming 1D surfels buffer
   vec4 data = surfels.data[globalIndex];

   //Transform point
   //Note must be a vec4 ending in 1 for matrix multiplication to work.
   vec4 point = perspective * data;

   //log(point.z + 1); //TODO

   //Perspective divide
   vec4 ndc = point / point.w;
//   point = point / point.z;

//   bool dscrd = ((point.x > 1.0) || (point.x < -1.0) ||
//		 (point.y > 1.0) || (point.y < -1.0) ||
//		 (point.z > 1.0) || (point.z < -1.0));
   
   const uint maxFpz = ~0;

   /*
   //Flip z (for purposes of atomicMax)
   uint pz = maxFpz - uint(fpz);
   
   //Endianness...?
   //NB (TODO?) this will make most values 0. Might have a discard pass
   uint mask = 1 << 24;

   uint z = pz & mask;
   
//   uint r = 1 >> 8;

//   uint value = z | r;

   uint value = z;
   */

   //Must figure out where the sign bit is for following 2's comp bit ops
   bool endianness = int(0x1) < int(~0x1);

   //Max -ve should be 1000... or ...0001.
   //Max +ve should be 0111... or ...1110.
   ivec2 minMax = ivec2(endianness? 0x1 : 0x1 << 31,
			endianness? ~0x1 : ~(0x1 << 31));

   vec2 minMaxf = vec2(minMax);

   //Check it won't over/underflow
//   dscrd = dscrd || ((point.x < minMaxf.x) || (point.x > minMaxf.x) ||
//		     (point.y < minMaxf.y) || (point.y > minMaxf.y));

   vec2 wind = vec2(float(samplesXY.x / 2) * ndc.x + float(samplesXY.x / 2),
		    float(samplesXY.y / 2) * ndc.y + float(samplesXY.y / 2));
		       
   ivec2 ptxy = ivec2(point.xy);

   ivec2 coords = getScreenCoords(ptxy);

//   ivec2 coords = ivec2(data.xy);

//   ivec2 coords = ivec2(-1, -1);
   
   //Depth followed by rgb
   imageAtomicMax(samples,
		  ivec2(wind),
//		  coords,
//		  (ptxy + ivec2(samplesXY) / 2),
//		  dscrd? ivec2(-1, -1) : coords,
//		  value);
//		  ~uint(0));
		  uint(0xff00));
//		  uint(data[1]));

//   imageStore(samples, coords, uvec4(~uint(0)));
}
