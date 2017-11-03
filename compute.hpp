#pragma once

#include <string>
#include <vector>
#include <cmath>

#define GEOM_CPP
#define GEOM_IMPL
#include "../geom/geom.h"

using namespace std;
using namespace geom;

class shader
{
private:
   string filename;

   GLenum kind;
   GLuint handle;

   string read();

   string getLogGL();
public:
   shader(const string& nm)
      : filename (nm)
      , kind (GL_COMPUTE_SHADER)
      , handle (0)
   {}

   ~shader() { quit(); }
   
   bool prep(GLuint program);
   void quit();
};

class program
{
private:
   GLuint handle;

   shader compute;

   string getLogGL();

public:
   program(const string& shaderNm)
      : handle (0)
      , compute (shaderNm)
   { prep(); }
   ~program() { quit(); }
   
   bool prep();
   void quit();

   void use();

   GLuint getHandle();
};

class framebuffer;

class image
{
private:
   GLuint handle;

   /*
     These are stored so you can sub-buffer (both in e.g. clear(),
     which will use glSubBufferData(), and in shaders, which can use
     x, y as uniforms to figure out accessing patterns. That way you
     don't have to reallocate when the image shrinks.
   */
   GLuint xy[2];
   GLint xyLoc;

   //Upload x, y as uniforms
   void pushSize();

public:
   image(GLint sizeLocation)
      : handle (0)
      , xyLoc (sizeLocation)
   { xy[0] = 0; xy[1] = 0; }
   ~image() { quit(); }
   
   void prep(GLuint width, GLuint height);
   void quit();

   void use(GLuint binding, GLenum access);
   void resize(GLuint width, GLuint height);
   void clear();
   void blit(framebuffer& fb);

   GLuint getHandle() { return handle; }
   float getAspectRatio() const;
};

class framebuffer
{
private:
   GLuint handle;
   
public:
   framebuffer() : handle (0) {};
   ~framebuffer() { quit(); }
   
   bool prep(image& img);
   void quit();

   void use();
   void blit(GLint width, GLint height);
};

class buffer
//Specifically in this case, a Shader Storage Buffer Object
{
private:
   GLuint handle;

   size_t nBytes;

public:
   buffer() : handle (0) , nBytes (0) {}
   ~buffer() { quit(); }
   
   void prep(vector<float> data, GLuint binding);
   void quit();

   void clear();

   size_t size() const { return nBytes; }
};

class surfelModel
{
private:
   buffer data;

   size_t getNumSurfels() const;

   //TODO preprocess non-surfel model here?

public:
   void prep(const string fileName, GLuint binding);
   
   void render(int localX, int localY);
};
