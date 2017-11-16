#pragma once

#include <string>
#include <vector>
#include <cmath>
#include <iostream>
#include <map>

#define GEOM_CPP
#include "../lib/geom/geom.h"

//This is so a single error can be inputted for places in a loop.
//Still won't work with multiple files yet (since a function in
//another file could be called in 2 places in the same loop and should
//perhaps be logged twice? TODO) so I've left in printErrorGL().
extern std::map<int, std::string> errorsGL;

std::string getErrorGL();
void printErrorGL(const char* file, int line);
void logErrorGL(int line);
void printErrorsGL();

bool getWkgpDimensions(uint32_t& xWkgps, uint32_t& yWkgps,
		       uint32_t localX, uint32_t localY,
		       uint32_t reqGlobalX, uint32_t reqGlobalY);

class shader
{
private:
   std::string filename;

   GLenum kind;
   GLuint handle;

   std::string read();

   std::string getLogGL();
public:
   shader(const std::string& nm);
   ~shader();
   
   bool prep(GLuint program);
   void quit();
};

class program
{
private:
   GLuint handle;

   shader compute;

   std::string getLogGL();

public:
   program(const std::string& shaderNm);
   ~program();
   
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
   image(GLint sizeLocation);
   ~image();
   
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
   framebuffer();
   ~framebuffer();
   
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
   buffer();
   ~buffer();
   
   void prep(std::vector<float> data, GLuint binding);
   void quit();

   void clear();

   size_t size() const { return nBytes; }
};

class surfelModel
{
private:
   buffer data;

   size_t getNumSurfels() const;

public:
   void prep(const std::string fileName, GLuint binding);
   
   void render(int localX, int localY);
};
