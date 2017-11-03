#include <glad/glad.h>

#include "compute.hpp"
#include "projection.hpp"
#include "pcdReader.hpp"
#include "sdl_utils.hpp"

#include <iostream>
#include <map>
#include <cstring>

string getErrorGL()
{
   GLint error = glGetError();

   switch (error)
   {
      case GL_INVALID_ENUM:
	 return "GL_INVALID_ENUM";
      case GL_INVALID_VALUE:
	 return "GL_INVALID_VALUE";
      case GL_INVALID_OPERATION:
	 return "GL_INVALID_OPERATION";
      case GL_INVALID_FRAMEBUFFER_OPERATION:
	 return "GL_INVALID_FRAMEBUFFER_OPERATION";
      case GL_OUT_OF_MEMORY:
	 return "GL_OUT_OF_MEMORY";

      case GL_NO_ERROR:
	 return string();
	 
      default:
	 return to_string(error);
   }
}

//This is so a single error can be inputted for places in a loop.
//Still won't work with multiple files yet (since a function in
//another file could be called in 2 places in the same loop and should
//perhaps be logged twice? TODO) so I've left in printErrorGL().
static map<int, string> errorsGL;

void printErrorGL(const char* file, int line)
{
   string err = getErrorGL();

   if (err.size()) { cerr << "GL error at " << file << ", " << line << ": " << err << endl; }
}

void logErrorGL(int line)
{
   string err = getErrorGL();

   //TODO? What if the error is different

   if (err.size())
   {
      if (!errorsGL.count(line))
      {
	 errorsGL[line] = err;
      }
   }
}

#define errorGL() printErrorGL(__FILE__, __LINE__)
#define LOG_GL() logErrorGL(__LINE__)

void printErrorsGL()
{
   for (auto& lineError : errorsGL)
   {
      cerr << "GL error at line " << lineError.first << ": "
	   << lineError.second << '\n';
   }

   cerr << flush;
}

string shader::read()
{
   ifstream file;

   file.open(filename, ifstream::in);

   file.seekg(0, file.end);
   size_t len = file.tellg();
   file.seekg(0, file.beg);
   
   string text;
   
   text.reserve(len);
   
#if 0
   /*
     Proof of concept: work out workgroup sizes (within shader),
     dynamically- have to, unless under the minimums.
     The minimums expected are 1024 total, 1024/1024/64.

     Note that this is not the same as the -number- of workgroups,
     which has similar limits in place you have to check - and for
     occupcancy's sake should depend on the dynamically-worked-out
     sizes here.
   */
   
   text += "#version 430\n";

   GLint maxWkgpSizeTotal = -1;
   glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxWkgpsTotal);

   GLint maxWkgpSizes[3];

   for (unsigned int i = 0; i < 3; ++i)
   {
      glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_SIZE, i, &maxWkgpSizes[i]);
   }
   
   string wkgpX, wkgpY, wkgpZ;
   
   //TODO: work out workgroup dimensions

   text += "layout(local_size_x = " + wkgpX +
      ", local_size_y = " + wkgpY +
      ", local_size_z = " + wkgpZ + ") in;\n";
#endif

   string line;

   while (getline(file, line))
   {
      text += line + '\n';
      
      continue;
   }
   
   if (!file.eof())
   {
      if (file.bad()) //Read/write error on I/O
      {
	 //TODO retry?

	 throw runtime_error("Read/write error on I/O");
      }

      else //Logical error on I/O
      {
	 throw runtime_error("Logical error on I/O");
      }
   }
   
   file.close();

   return move(text);
}

string shader::getLogGL()
{
   GLint len = 0;
   glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &len);

   char cstr[len];
   memset(cstr, ' ', len);
   
   glGetShaderInfoLog(handle, len, NULL, cstr);

   string result = string(cstr);

   //Remove \0
   result.pop_back();

   return move(result);
}

bool shader::prep(GLuint program)
{
   string text = read();

   handle = glCreateShader(kind);

   const GLchar* source = text.c_str();

   glShaderSource(handle,
		  1, //count
		  (const GLchar**) &source,
		  nullptr);

   glCompileShader(handle);
   
   GLint checkCompile = 0;
   glGetShaderiv(handle, GL_COMPILE_STATUS, &checkCompile);

   if (!checkCompile)
   {
      //TODO throw, log, whatever

      cerr << "Shader \"" + filename + "\" failed to compile";

      string infoLog = getLogGL();

      if (infoLog.size()) { cerr << ":" << endl << infoLog << endl; }

      else { cerr << endl; }

      return false;
   }

   GLint prevAttached = 0;
   glGetProgramiv(program, GL_ATTACHED_SHADERS, &prevAttached);
   
   glAttachShader(program, handle);

   GLint checkAttach = 0;
   glGetProgramiv(program, GL_ATTACHED_SHADERS, &checkAttach);

   if (prevAttached == checkAttach)
   {
      //TODO Move to program::...?
      
      GLint len = 0;
      glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);

      cerr << "Shader \"" + filename + "\" failed to be attached";

      //(len includes \0) //TODO isolate this elsewhere
      if (len - 1)
      {
	 #if 0
	 char infoLog[len];
	 memset(infoLog, ' ', len);
	 
	 glGetProgramInfoLog(program, len, NULL, infoLog);

	 cerr << ":" << endl << infoLog << endl;
	 #endif

	 errorGL();
      }

      else { cerr << endl; }
      
      //TODO log etc.

      return false;
   }

   return true;
}

void shader::quit()
{
   glDeleteShader(handle);
}

string program::getLogGL()
{
   GLint len = 0;
   glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &len);

   //Confusingly, reported length doesn't include \0 if 0
   if (!len) { return string(); }

   char infoLog[len];
   
   glGetProgramInfoLog(handle, len, NULL, infoLog);

   string result = string(infoLog);

   //Remove \0
   result.pop_back();
   
   return move(result);
}

bool program::prep()
{
   handle = glCreateProgram();

   compute.prep(handle);

   glLinkProgram(handle);

   GLint checkLink = 0;
   glGetProgramiv(handle, GL_LINK_STATUS, &checkLink);

   if (!checkLink)
   {
      /*TODO log etc.*/

      cerr << "Program failed to link";

      string infoLog = getLogGL();
      
      if (infoLog.size()) { cerr << ":" << endl << infoLog << endl; }

      else { cerr << endl; }

      LOG_GL();

      return false;
   }

   //Validation? (TODO?)

   return true;
}

void program::quit()
{
   compute.quit();
   
   glDeleteProgram(handle);

   glUseProgram(0);
}

void program::use()
{
   glUseProgram(handle);
}

GLuint program::getHandle() { return handle; }

bool framebuffer::prep(image& img)
{
   glGenFramebuffers(1, &handle);
   glBindFramebuffer(GL_FRAMEBUFFER, handle);

   glFramebufferTexture2D(GL_FRAMEBUFFER,
			  GL_COLOR_ATTACHMENT0,
			  GL_TEXTURE_2D,
			  img.getHandle(),
			  0);

   glBindFramebuffer(GL_FRAMEBUFFER, 0);

   return true;
}

void framebuffer::use()
//ie use to read from for blitting
{
   glBindFramebuffer(GL_READ_FRAMEBUFFER, handle);
}

void framebuffer::blit(GLint width, GLint height)
{
   //This depends on glDrawBuffer() and glReadBuffer() but they aren't
   //variable (even after swapping buffers) so do it once, outside of the function.
   glBlitFramebuffer(0, 0, width, height, //src
		     0, 0, width, height, //dst
		     GL_COLOR_BUFFER_BIT,
		     GL_LINEAR);
}

void framebuffer::quit()
{
   glDeleteFramebuffers(1, &handle);
}

void image::prep(GLuint width, GLuint height)
{
   glGenTextures(1, &handle);
   glBindTexture(GL_TEXTURE_2D, handle);

   LOG_GL();

   glTexImage2D(GL_TEXTURE_2D,
		0, //lod
		GL_RGBA,
		width,
		height,
		0, //border
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		nullptr);

   //

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   LOG_GL();

   //TODO checks

   //Size uniforms
   xy[0] = width; xy[1] = height;
   
   //Locations: don't bother with glGetUniformLocation, that'd require
   //a string. Just use a layout() qualifier in the shader.
   //(So xLoc and yLoc will be part of the constructor.)
   
   pushSize();

   LOG_GL();

   //Using nullptr above makes it 'uninitialised'
   clear();
}

void image::quit()
{
   glDeleteTextures(1, &handle);
}

void image::use(GLuint binding, GLenum access)
{
   //This 'binds' 'images' to 'texture units'.
   //Formats are ok with different in-shader accesses as long as
   //they're 'compatible'. R32UI (ie a 32bit red) is compatible with
   //RGBA8UI (ie a 32bit vector) - it's mainly about size.

   glBindImageTexture(binding,
		      handle,
		      0, //level
		      GL_FALSE, //layered
		      0, //layer
		      access,
		      GL_RGBA8UI); //Could benefit from another param
}

void image::resize(GLuint width, GLuint height)
{
   /*
     Overcomplicated method of reducing time spent resizing:
     If the requirements -shrink-, then no need to reallocate entire
     image; can just consistently use SubBuffer.

     NB This precludes using imageSize in a shader. There must be
     uniforms with the size, instead. Hence pushSize().
   */

   bool needRealloc = (width > xy[0]) || (height > xy[1]);

   xy[0] = width; xy[1] = height;
   
   if (needRealloc)
   {
      //Must bind for glTexImage2D!
      glBindTexture(GL_TEXTURE_2D, handle);

      glTexImage2D(GL_TEXTURE_2D,
		   0, //lod
		   GL_RGBA,
		   width,
		   height,
		   0, //border
		   GL_RGBA,
		   GL_UNSIGNED_BYTE,
		   nullptr);

      //Clear the newly allocated parts
      clear();
   }

   pushSize();
}

void image::pushSize()
{
   //NB: uniforms have to be done while a shader (the right one) is being used.

   glUniform2uiv(xyLoc, 1, xy);

   LOG_GL();
}

void image::clear()
{
   //Unusually (?) RGBA reads 4 uints here.
   //Best not use GL_RED; it puts alpha to 1.0.
   
   static const uint32_t clearValue[4] = {0, 0, 0, 0};
   
   glClearTexSubImage(handle,
		      0, //level
		      0, 0, 0, //origin x,y,z
		      xy[0], xy[1], 1, //1: depth
		      GL_RGBA,
		      GL_UNSIGNED_INT,
		      clearValue);
}

void image::blit(framebuffer& fb)
{
   GLint wid, hei;

   wid = (GLint) xy[0]; hei = (GLint) xy[1];

   fb.blit(wid, hei);
}

float image::getAspectRatio() const
{
   return (float) xy[0] / (float) xy[1];
}

void buffer::prep(vector<float> data, GLuint binding)
{
   nBytes = sizeof(float) * data.size();

   glGenBuffers(1, &handle);

   glBindBuffer(GL_SHADER_STORAGE_BUFFER, handle);

   //Round up size in bytes for packing (for benefit of shader
   //accesses)? But then - SSBOs work out the size of
   //indeterminately-sized arrays at shader invocation time anyway...
   
   glBufferData(GL_SHADER_STORAGE_BUFFER,
		data.size() * sizeof(float),
		data.data(),
		GL_STATIC_DRAW);

   //This is the actually important binding - to a place explicitly
   //mentioned by the shader.
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, handle);
}

void buffer::quit()
{
   glDeleteBuffers(1, &handle);
}

void buffer::clear()
{
   //Value given depends on current pipeline (of depth being in x etc,
   //and on the format of the buffer being a bunch of (32b) uvec4s)

   //0 will mean both the furthest depth possible, and a colour value
   //of (0, 0, 0) i.e. black

   GLuint value = 0;
   
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, handle);
   glClearBufferData(handle,
		     GL_RGBA32UI, //dst format
		     GL_RGBA, //src swizzle
		     GL_UNSIGNED_INT, //src type
		     &value);

   //TODO check (at least in debug)
}

void surfelModel::prep(const string fileName, GLuint binding)
{
   if (!fileName.size()) { throw invalid_argument("No file name given for surfel model"); }

   pcdReader pcd;

   pcd.prep(fileName);

   vector<float> load = pcd.read();

   if (load.size() % 4) { throw invalid_argument("Surfel input data not divisible into groups of 4 floats"); }

   data.prep(load, binding);
}

size_t surfelModel::getNumSurfels() const
{
   //NB buffer::size() gives bytes (confusingly! TODO)
   return (data.size() / sizeof(float)) / 4;
}

bool getWkgpDimensions(uint32_t& xWkgps, uint32_t& yWkgps,
		       uint32_t localX, uint32_t localY,
		       uint32_t reqGlobalX, uint32_t reqGlobalY)
{
   /*
     Can't simply rely on the sum total threads needed and the sum
     total of the workgroup size, because coords will be affected by
     the local size along y, and because (in the case of screens being
     rendered to) there will be specific needs for y dimensionality in
     global invocations.

     (It'd be ok if you agreed to keep workgroup size 1D. But in the 
     case of image processing in 'pixels' buffer, there's an obvious 
     advantage for occupancy available.)

     NB. GL limits involve max numbers of workgroups along
     dimensions. Assuming the shader is using the invocation index as
     part of a coordinate, it's not very helpful to know that you're
     over the max. You couldn't do a second pass since there's no
     'base index' in compute shaders; you couldn't start from a higher
     index so you could fit the difference between required and max
     into the max.
     You could do this if you put in a base index as a uniform. Not
     sure how that'd work with 'dynamically uniform' accesses etc., though.

     Potential TODO: this is a potentially wasteful approach. Ideally
     you'd want to change the local sizes within the shader,
     dynamically, too.

     What if you only want 1D (and therefore call this
     with (someNumber, 1)) but the shader has a local y of 64?
     I don't think this is avoidable though - in that case, the shader
     would have coordinates that depended on their local y. You
     couldn't reduce the number of x's dispatched because you couldn't
     substitute y for x.
     Note that since the shader gets the workgroup numbers as in-built
     variables, that would be the place to work around that.
   */

   //These aren't the max globals (ie # invocations). They're max
   //-numbers- of workgroups.
   //These would be better done once and saved, etc.
   
   GLint maxXWkgps, maxYWkgps;

   glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT,
		   0,
		   &maxXWkgps);

   glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT,
		   1,
		   &maxYWkgps);

   //Add 1 if it doesn't cleanly divide. (1 because division rounds down)
   xWkgps = reqGlobalX / localX + ((reqGlobalX % localX)? 1 : 0);
   yWkgps = reqGlobalY / localY + ((reqGlobalY % localY)? 1 : 0);

   if ((xWkgps > (uint32_t) maxXWkgps) || (yWkgps > (uint32_t) maxYWkgps))
   {
      return false;
   }

   else { return true; }
}

void surfelModel::render(int localX, int localY)
{
   uint32_t xWkgps, yWkgps;
      
   getWkgpDimensions(xWkgps, yWkgps,
		     (uint32_t) localX, (uint32_t) localY,
		     getNumSurfels(), 1);

   glDispatchCompute(xWkgps,
		     yWkgps,
		     1);
}

int main(int argc, char** args)
{
   SDL_SetMainReady();

   int winX = 960; int winY = 540;
   
   sdlInstance instance = sdlInstance(winX, winY);

   program surfelsToSamples = program("surfelsToSamples.c.glsl");   
   program samplesToPixels = program("samplesToPixels.c.glsl");

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
   string fileName;
   
   if (argc > 1)
   {
      fileName = args[1];
   }

   else fileName = "ism_train_horse.pcd";

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
		       vec3(0.0, 0.0, -500.0), //pos
		       vec3(0.0, 0.0, 1.0), //dirZ
		       vec3(0.0, 1.0, 0.0), //dirY
		       35.f, //horizontal fov
		       pixels.getAspectRatio(), //aspect ratio
		       1.f, 10.f); //near, planes z

   LOG_GL();

   surfelsToSamples.use();
   cam.pushTransformMatrix();

   LOG_GL();

   while (!instance.getQuit())
   {
      bool cameraMoved = false;

      instance.pollEvents();

      const float speed = 5.f;
      const float angle = 0.0005;
      
      if (uint32_t numW = instance.takeNumW())
      {
	 vec3 pos = cam.getPos();

	 pos = pos + cam.getZ() * (float) numW * speed;

	 cam.setPos(pos);

	 cameraMoved = true;
      }

      if (uint32_t numS = instance.takeNumS())
      {
	 vec3 pos = cam.getPos();

	 pos = pos - cam.getZ() * (float) numS * speed;

	 cam.setPos(pos);

	 cameraMoved = true;
      }

      if (uint32_t numA = instance.takeNumA())
      {
//	 vec3 axisRot = vec3(0.f, 1.f, 0.f);
	 vec3 axisRot = cam.getY();

	 float finalDegrees = -angle * numA;
	 
	 cam.rotate(axisAngle(axisRot, finalDegrees));

	 cameraMoved = true;
      }

      if (uint32_t numD = instance.takeNumD())
      {
	 cam.rotate(axisAngle(vec3(0.f, 1.f, 0.f), angle * numD));

	 cameraMoved = true;
      }

      //If the window's size has changed, size of buffers must change
      //with it.      
      if (instance.hasWindowChanged(winX, winY))
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

      surfelsToSamples.use(); LOG_GL();

      if (cameraMoved) { cam.pushTransformMatrix(); }

      LOG_GL();

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
