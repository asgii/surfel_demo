#include "../lib/glad/include/glad/glad.h"

#define GEOM_CPP
#define GEOM_IMPL
#include "../lib/geom/geom.h"

#include "compute.hpp"
#include "projection.hpp"
#include "pcdReader.hpp"
#include "sdl_utils.hpp"

#include <cstring>

#define errorGL() printErrorGL(__FILE__, __LINE__)
#define LOG_GL() logErrorGL(__LINE__)

bool
handleEvents(sdlInstance& inst, camera& cam)
{
   bool cameraMoved = false;

   uint32_t numW = inst.takeNumW();
   uint32_t numS = inst.takeNumS();
      
   if (numW or numS)
   {
      bool forward = numW > numS;

      float sum = max(numW, numS) - min(numW, numS);

      cam.moveZ(sum, forward);
	 
      cameraMoved = true;
   }

   uint32_t numA = inst.takeNumA();
   uint32_t numD = inst.takeNumD();

   if (numA or numD)
   {
      bool cw = numD > numA;

      uint32_t sum = max(numD, numA) - min(numD, numA);

      cam.rotateY(sum, cw);

      cameraMoved = true;
   }

   return cameraMoved;
}

bool
handleWindowResize(sdlInstance& inst,
		   int& winX, int& winY,
		   image& samples, image& pixels)
{
   bool cameraMoved = false;
   
   if (inst.hasWindowChanged(winX, winY))
   {
      //These also do associated uniforms
      samples.resize(winX * 2, winY * 2);
      pixels.resize(winX, winY);

      //Don't update aspect ratio based on new sizes though - it's weird.

      //Perspective matrix will need re-uniforming since it
      //will have aspect ratio baked in
      cameraMoved = true;
	 
      //Framebuffer seems not to need re-connecting to texture, either.
   }

   return cameraMoved;
}

int main(int argc, char** args)
{
   SDL_SetMainReady();

   int winX = 960; int winY = 540;
   
   sdlInstance instance = sdlInstance(winX, winY);

   program surfelsToSamples = program("resources/shaders/surfelsToSamples.c.glsl");
   
   program samplesToPixels = program("resources/shaders/samplesToPixels.c.glsl");

   LOG_GL();

   //Get the local sizes from those shaders.
   int surfelsToSamplesSizes[3];
   int samplesToPixelsSizes[3];
   glGetProgramiv(surfelsToSamples.getHandle(), GL_COMPUTE_WORK_GROUP_SIZE, surfelsToSamplesSizes);
   glGetProgramiv(samplesToPixels.getHandle(), GL_COMPUTE_WORK_GROUP_SIZE, samplesToPixelsSizes);

   //Programs have to be used while uniforms are loaded
   surfelsToSamples.use();
   //The first 2 arguments are ad hoc bindings, from the shaders - for wid/height
   image samples = image(3);
   
   LOG_GL();

   samplesToPixels.use();
   image pixels = image(5);

   LOG_GL();

   surfelsToSamples.use();
   samples.prep(winX * 2, winY * 2);

   samplesToPixels.use();
   pixels.prep(winX, winY);

   LOG_GL();

   //Bind images to texture units
   samples.use(1, GL_READ_WRITE);
   pixels.use(2, GL_WRITE_ONLY);

   LOG_GL();

   surfelModel surfels; LOG_GL();

   const GLuint surfelsBinding = 3;
   std::string fileName = "resources/models/";
   
   if (argc > 1)
   {
      fileName = fileName + args[1];
   }

   else fileName = "resources/models/ism_train_horse.pcd";

   try
   {
      surfels.prep(fileName, surfelsBinding);
   }
   
   catch (const exception& err)
   {
      cerr << err.what() << endl;

      return 1;
   }

   LOG_GL();

   //Framebuffer stuff
   framebuffer frame; LOG_GL();
   
   frame.prep(pixels); LOG_GL();

   frame.use(); LOG_GL();

   //Uniform stuff for the first time (e.g. perspective matrix)
   GLint perspectiveLoc = glGetUniformLocation(surfelsToSamples.getHandle(),
					       "perspective");

   LOG_GL();

   camera cam = camera(perspectiveLoc,
		       geom::vec3(0.0, 0.0, -500.0), //pos
		       geom::vec3(0.0, 0.0, 1.0), //dirZ
		       geom::vec3(0.0, 1.0, 0.0), //dirY
		       35.f, //horizontal fov
		       pixels.getAspectRatio(), //aspect ratio
		       1.f, 1000.f); //near, planes z

   LOG_GL();

   surfelsToSamples.use();
   cam.pushTransformMatrix();

   LOG_GL();

   while (!instance.getQuit())
   {
      instance.pollEvents();

      //
      bool cameraMoved = handleEvents(instance, cam);

      //If the window's size has changed, size of buffers must change
      //with it.
      cameraMoved = (handleWindowResize(instance,
					winX, winY,
					samples, pixels) or
		     cameraMoved);

      surfelsToSamples.use(); LOG_GL();
      if (cameraMoved) { cam.pushTransformMatrix(); } LOG_GL();

      surfels.render(surfelsToSamplesSizes[0], surfelsToSamplesSizes[1]); LOG_GL();

      //Block until all image ops in the previous shader are done
      //(more or less).
      glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

      samplesToPixels.use(); LOG_GL();

      //TODO check workgroup maximums
      uint32_t xWkgps, yWkgps;
      
      getWkgpDimensions(xWkgps, yWkgps,
			samplesToPixelsSizes[0], samplesToPixelsSizes[1],
			winX, winY);

      glDispatchCompute(xWkgps,
			yWkgps,
			1);

      LOG_GL();

      glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

      pixels.blit(frame);

      glFlush(); //Executes any lazy command buffers
      glFinish(); //Blocks til they're done

      instance.swapWindow(); LOG_GL();

      //Clear for next frame
      samples.clear();
      pixels.clear();

      LOG_GL();
      
      glClear(GL_COLOR_BUFFER_BIT); LOG_GL();
   }

   printErrorsGL();

   return 0;
}
