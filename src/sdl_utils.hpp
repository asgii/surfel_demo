#pragma once

#include <SDL2/SDL.h>

class sdlInstance
{
private:
   SDL_Window* window;
   SDL_GLContext context;

   SDL_Event event;

   int windowX, windowY;

   //Last frame and this frame
   uint32_t then, now;
   uint32_t lengthFrame;

   void updateLengthFrame();

   //Following stuff is event-handling specific; could be in its own
   //class
   bool key_quit;
   uint32_t key_w, key_s, key_a, key_d;
   uint32_t num_w, num_s, num_a, num_d;

   int mouseX, mouseY;
   int mouseDX, mouseDY;

   bool panning;
   //Mouse coordinates when panning started.
   int panningX, panningY;

   //Start panning with this mouse event
   void startPanning(const SDL_Event& event);
   //Pan til this mouse event
   void completePanning(const SDL_Event& event);
   //Pan til end of the frame; don't stop panning
   void completePanning();

   //SDL_GetError is meant only to be valid after a failing SDL
   //call. So it makes sense that it's private.
   const char* getError();

   void prep();
   void quit();

public:
   sdlInstance(int winX, int winY);

   void swapWindow();
   
   bool hasWindowChanged(int& x, int& y);
   void getMouseDelta(int& x, int& y) const;
   
   void pollEvents();

   uint32_t getLengthFrame() const;
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
};
