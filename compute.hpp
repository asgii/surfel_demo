#pragma once

#include <SDL2/SDL.h>

#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#define SCRN_WIDTH 1920
#define SCRN_HEIGHT 1080

using namespace std;

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

public:
   sdlInstance()
      : window (nullptr)
      , windowX (SCRN_WIDTH)
      , windowY (SCRN_HEIGHT)
   { prep(); }

   void swapWindow();
   bool checkQuit();
   bool getWindowSize(int& x, int& y);
   void getError();
   
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
};

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
   GLsizei x, y;
   GLint xLoc, yLoc;

   //Upload x, y as uniforms
   void pushSize();

public:
   image(GLint widthLocation, GLint heightLocation)
      : handle (0)
      , x (0)
      , y (0)
      , xLoc (widthLocation)
      , yLoc (heightLocation)
   {}
   ~image() { quit(); }
   
   void prep(GLsizei width, GLsizei height);
   void quit();

   void use(GLuint binding);
   void resize(GLsizei width, GLsizei height);
   void clear();

   GLuint getHandle() { return handle; }
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
