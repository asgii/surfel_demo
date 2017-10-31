#pragma once

#include <SDL2/SDL.h>

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cmath>

#define GEOM_CPP
#include "../geom/geom.h"

#define SCRN_WIDTH 960
#define SCRN_HEIGHT 540

using namespace std;
using namespace geom;

class pcdReader
{
private:
   ifstream file;
   istringstream line; //Buffer for line
   string word; //Buffer for a word in the line

   const string& getWord();
   const string& getWordOnLine();
   string getLine();
   
   bool seekWord(string str);

   //For restoring place in file in failed ops
   void getPlace(streampos& fileSave, string& lineSave, string& wordSave);
   void setPlace(streampos fileSave, string lineSave, string wordSave);

   size_t readHeader();
   vector<float> readBody(size_t numSurfels);

public:
   ~pcdReader() { file.close(); }
   
   void prep(const string filename);
   
   vector<float> read();
};

class sdlInstance
{
private:
   SDL_Window* window;
   SDL_GLContext context;

   SDL_Event event;

   int windowX, windowY;

   //Last frame and this frame
   uint32_t then, now;

   //Following stuff is event-handling specific; could be in its own
   //class
   bool key_quit;
   uint32_t key_w, key_s, key_a, key_d;
   uint32_t num_w, num_s, num_a, num_d;

public:
   sdlInstance()
      : window (nullptr)
      , windowX (SCRN_WIDTH), windowY (SCRN_HEIGHT)
      , then (0), now(0)
      , key_quit (false)
      , key_w (0), key_s (0), key_a (0), key_d (0)
      , num_w (0), num_s (0), num_a (0), num_d (0)
   { prep(); }

   void swapWindow();
   bool getWindowSize(int& x, int& y);
   void getError();
   void pollEvents();

   bool getQuit() const;

   //'take' because they're not const; they subtract values
   uint32_t takeW();
   uint32_t takeS();
   uint32_t takeA();
   uint32_t takeD();

   uint32_t takeNumW();
   uint32_t takeNumS();
   uint32_t takeNumA();
   uint32_t takeNumD();
   
   void prep();
   void quit();
};

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
   
   void render();
};

class frustum
{
protected:
   vec3 pos; //Position
   vec3 dirZ, dirY; //Z and Y axes (rotation)
   float horFov, verFov; //Fields of view
   float nearDZ, planesDZ; //distance from position to near plane;
			   //from near plane to far plane

   float getFarDZ() const;
   vec3 getDirX() const;

   //TODO: remove this
   bool tested;

public:
   frustum(vec3 nuPos, vec3 nuDirZ, vec3 nuDirY,
	   float nuHorFov, float aspRatio,
	   float nuNearDZ, float nuPlanesDZ)
      : pos(nuPos)
      , dirZ (nuDirZ), dirY (nuDirY)
      ,	horFov (radians(nuHorFov / 2.f))
      , verFov (atan(tan(horFov) / aspRatio))
      ,	nearDZ (nuNearDZ), planesDZ (nuPlanesDZ)
      , tested (false) //TODO remove
   {}

   mat4 getInverseTransformMatrix() const;
   mat4 getPerspectiveMatrix() const;

   vec3 getPos() const;
   void setPos(vec3 nuPos);

   vec3 getZ() const;
   vec3 getY() const { return dirY; } //TODO remove
};

class camera : public frustum
{
protected:
   GLint transformLoc;
   
public:
   camera(GLint transfLoc, vec3 nuPos, vec3 nuDirZ, vec3 nuDirY,
	  float nuHorFov, float aspRatio,
	  float nuNearDZ, float nuPlanesDZ)
      : frustum(nuPos, nuDirZ, nuDirY,
		nuHorFov, aspRatio,
		nuNearDZ, nuPlanesDZ)
      , transformLoc (transfLoc)
   {}
   
   void pushTransformMatrix();

   void rotate(axisAngle aa);
};
