#include "../lib/glad/include/glad/glad.h"

#include "sdl_utils.hpp"

#include <iostream>

using namespace std;

sdlInstance::sdlInstance(int winX, int winY)
   : window (nullptr)
   , windowX (winX), windowY (winY)
   , then (0), now(0)
   , lengthFrame (0)
   , key_quit (false)
   , key_w (0), key_s (0), key_a (0), key_d (0)
   , num_w (0), num_s (0), num_a (0), num_d (0)
   , mouseX (0), mouseY (0)
   , mouseDX (0), mouseDY (0)
   , panning (false)
   , panningX (0), panningY (0)
{ prep(); }

const char*
sdlInstance::getError()
{
   const char* result = SDL_GetError();

   SDL_ClearError();

   return result;
}

void
sdlInstance::prep()
{
   if (SDL_Init(SDL_INIT_VIDEO) != 0)
   {
      cerr << "SDL failed to initialise: \'" << getError() << endl;
   }

   //4.4 for glClearTex(Sub)Image
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);

   SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
		       SDL_GL_CONTEXT_PROFILE_CORE);

   uint32_t flags = (SDL_WINDOW_OPENGL |
		     SDL_WINDOW_RESIZABLE);

   window = SDL_CreateWindow("compute test",
			     
			     SDL_WINDOWPOS_CENTERED,
			     SDL_WINDOWPOS_CENTERED,
			     
			     windowX,
			     windowY,
			     
			     flags);

   if (!window)
   {
      //TODO throw?

      cerr << "SDL failed to create window: \'" << getError() << endl;
   }

   context = SDL_GL_CreateContext(window);

   if (!context)
   {
      cerr << "SDL failed to create OpenGL context: \'" << getError() << endl;
   }

   if (SDL_GL_SetSwapInterval(1) == -1)
   {
      //TODO

      cerr << getError() << endl;
   }

   //Afaik this gives Glad the function by which it's to retrieve
   //pointers to specific GL functions, and makes it do so for all it
   //needs.
   //TODO figure out how to error check the SDL version, since this
   //one is actually void...!
   if (!gladLoadGL())
   //if (!gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress))
   {
      //TODO throw, etc.
      
      cerr << "Glad failed to load some OpenGL function." << endl;
   }

   then = SDL_GetTicks();
}

void
sdlInstance::quit()
{
   SDL_DestroyWindow(window);
   SDL_GL_DeleteContext(context);

   SDL_Quit();
}

void
sdlInstance::swapWindow()
{
   SDL_GL_SwapWindow(window);
}

void
sdlInstance::updateLengthFrame()
{
   uint32_t sum = now - then;

   //Underflow protection
   lengthFrame = (sum > now) ? 0 : sum;
}

uint32_t
sdlInstance::getLengthFrame() const
{
   return lengthFrame;
}

void
sdlInstance::startPanning(const SDL_Event& event)
{
   panning = true;

   panningX = event.button.x;
   panningY = event.button.y;
}

void
sdlInstance::completePanning(const SDL_Event& event)
{
   mouseX = event.button.x;
   mouseY = event.button.y;
   
   mouseDX = panningX - mouseX;
   mouseDY = panningY - mouseY;
   
   panning = false;
}

void
sdlInstance::completePanning()
{
   SDL_GetMouseState(&mouseX, &mouseY);

   mouseDX = panningX - mouseX;
   mouseDY = panningY - mouseY;

   //Update panning x,y too - to reflect that you've put them in
   //dx,dy
   panningX = mouseX;
   panningY = mouseY;

   //Don't end panning; this fn is for when panning continues through
   //end of frame
}

void
sdlInstance::pollEvents()
//General-purpose event registering
{
   now = SDL_GetTicks();

   updateLengthFrame();

   //Assuming they're handled every frame
   mouseDX = 0; mouseDY = 0;

   while (SDL_PollEvent(&event))
   {
      switch (event.type)
      {
	 case SDL_QUIT:
	 { key_quit = true; }
	 break;

	 case SDL_MOUSEBUTTONDOWN:
	 {
	    switch (event.button.button)
	    {
	       case SDL_BUTTON_LEFT:
	       {
		  startPanning(event);
	       }
	       break;
	    }
	 }
	 break;

	 case SDL_MOUSEBUTTONUP:
	 {
	    switch (event.button.button)
	    {
	       case SDL_BUTTON_LEFT:
	       {
		  completePanning(event);
	       }
	       break;
	    }
	 }
	 break;
	 
	 case SDL_KEYDOWN:
	 {
	    switch (event.key.keysym.sym)
	    {
	       case SDLK_ESCAPE:
	       { key_quit = true; }
	       break;

	       case SDLK_w:
	       { key_w += now - event.key.timestamp; ++num_w; }
	       break;
	       case SDLK_s:
	       { key_s += now - event.key.timestamp; ++num_s; }
	       break;
	       case SDLK_a:
	       { key_a += now - event.key.timestamp; ++num_a; }
	       break;
	       case SDLK_d:
	       { key_d += now - event.key.timestamp; ++num_d; }
	       break;
	    }
	 }
	 break;

	 case SDL_KEYUP:
	 {
	    uint32_t dt = now - event.key.timestamp;
	    
	    switch (event.key.keysym.sym)
	    {
	       //Careful of overflow
	       case SDLK_w:
	       {
		  if (dt >= key_w) { key_w = 0; }
		  else { key_w -= dt; }
	       }
	       break;
	       case SDLK_s:
	       {
		  if (dt >= key_s) { key_s = 0; }
		  else { key_s -= dt; }
	       }
	       break;
	       case SDLK_a:
	       {
		  if (dt >= key_a) { key_a = 0; }
		  else { key_a -= dt; }
	       }
	       break;
	       case SDLK_d:
	       {
		  if (dt >= key_d) { key_d = 0; }
		  else { key_d -= dt; }
	       }
	       break;
	    }
	 }
	 break;
      }
   }

   //If still panning (click has not been released during
   //frame), rotate
   if (panning)
   {
      completePanning();
   }

   then = now;
}

bool
sdlInstance::getQuit() const { return key_quit; }

uint32_t
sdlInstance::takeW()
{
   uint32_t result = key_w;

   key_w = 0;

   return result;
}

uint32_t
sdlInstance::takeS()
{
   uint32_t result = key_w;

   key_s = 0;

   return result;
}

uint32_t
sdlInstance::takeA()
{
   uint32_t result = key_a;

   key_a = 0;

   return result;
}

uint32_t
sdlInstance::takeD()
{
   uint32_t result = key_d;

   key_d = 0;

   return result;
}

uint32_t sdlInstance::takeNumW() { uint32_t result = num_w; num_w = 0; return result; }
uint32_t sdlInstance::takeNumS() { uint32_t result = num_s; num_s = 0; return result; }
uint32_t sdlInstance::takeNumA() { uint32_t result = num_a; num_a = 0; return result; }
uint32_t sdlInstance::takeNumD() { uint32_t result = num_d; num_d = 0; return result; }

bool
sdlInstance::hasWindowChanged(int& x, int& y)
{
   int oldX = windowX;
   int oldY = windowY;

   //Despite the fact that SDL_CreateWindow uses pixels,
   //SDL_GetWindowSize doesn't.
   SDL_GL_GetDrawableSize(window, &windowX, &windowY);

   //This saves time doing getWindowX() etc. (needed for image sizing)
   x = windowX;
   y = windowY;

   //Whether it's changed
   return (oldX == windowX) && (oldY == windowY);
}

void
sdlInstance::getMouseDelta(int& x, int& y) const
{
   x = mouseDX; y = mouseDY;
}
