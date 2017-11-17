#version 430

//1D on the basis that the buffer is 1D
layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout (std430, binding = 3) buffer surfelsBlock
{
   vec4 data[];
} surfels;

uniform mat4 perspective;

layout (r32ui, binding = 1) uniform uimage2D samples;

//Not imageSize() because of dubious resizing technique - see class image
layout (location = 3) uniform uvec2 samplesXY;

uint get1DGlobalIndex()
{
   /*
     The spec only provides a -local- 1D index
     (gl_LocalInvocationIndex), ie, local to the workgroup. I want to
     cover all elements of the surfels buffer with this shader - so it
     must be global.

     This is slight overkill for this shader since the local sizes
     (above) are only in x.
   */

   //The index of the start of this workgroup...
   uint base = (gl_WorkGroupID.z * gl_WorkGroupSize.z * gl_WorkGroupSize.x * gl_WorkGroupSize.y +
		gl_WorkGroupID.y * gl_WorkGroupSize.y * gl_WorkGroupSize.x +
		gl_WorkGroupID.x * gl_WorkGroupSize.x);

   //...plus the index into the workgroup.
   return base + gl_LocalInvocationIndex;
}

ivec2 getWindowCoords(vec2 ndc)
{
   /*
     Note input must be vec2, not ivec2.
     Since normalised device coords are [-1,1], casting to int before
     multiplying will make them -1, 0 or 1.
   */

   vec2 halfDimensions = vec2(samplesXY / 2);

   /*
     [-1,1] * halfDimensions = [-halfDimensions,halfDimensions]
     + halfDimensions = [0,samplesXY] (which is appropriate for image ops)
   */
   vec2 coords = halfDimensions * (ndc + 1);

   return ivec2(coords);
}

void main()
{
   vec4 data = surfels.data[get1DGlobalIndex()];

   //Transform point
   //Note must be a vec4 ending in 1.0 for matrix multiplication to work.
   vec4 point = perspective * data;

   //Perspective divide -> normalised device coords
   vec4 ndc = point / point.w;

   //z clip
   bool dscrd = abs(ndc.z) > 1.0;
   
   //Flip z (for purposes of atomicMax)
   uint value = ~0 - uint(point.w);

   ivec2 coords = getWindowCoords(ndc.xy);
   
   //Depth followed by rgb
   imageAtomicMax(samples,
//		  coords,
		  dscrd? ivec2(-1, -1) : coords,
		  value);
}
