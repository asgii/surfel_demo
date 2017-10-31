#include <glad/glad.h>
#include "compute.hpp"
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
      default:
	 return string();
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

void pcdReader::prep(const string filename)
{
   if ((filename.size() < 5) ||
       //-1 to get to last, -3 to get to '.'
       (filename.substr(filename.size() - 4) != ".pcd"))
   { throw invalid_argument("File name given, \"" + filename + "\", was not a .pcd"); }

   file.open(filename, ifstream::in);

   if (file.fail())
   {
      file.close();
      
      throw invalid_argument("No such .pcd file \"" + filename + "\", or problem reading file");
   }

   getLine();
   getWord();
}

void pcdReader::getPlace(streampos& fileSave,
			 string& lineSave,
			 string& wordSave)
{
   lineSave = line.str();
   wordSave = word;

   fileSave = file.tellg();
}

void pcdReader::setPlace(streampos fileSave,
			 string lineSave,
			 string wordSave)
{
   file.seekg(fileSave);

   line.str(lineSave);

   word = wordSave;
}

string pcdReader::getLine()
{
   //Important: clear old one! .str() does not
   line = istringstream();

   string str;

   //NB >> from stream to stream does the whole stream.
   getline(file, str);

   line.str(str);

   return line.str();
}

const string& pcdReader::getWordOnLine()
//Get a word from the current line only
{
   line >> word;

   return word;
}

const string& pcdReader::getWord()
{
   if (line.eof())
   {
      getLine();
   }

   line >> word;

   return word;
}

bool pcdReader::seekWord(string str)
//Seek word -after- str, in the stream.
{
   //Clearly, this won't work if str includes ' ' or '\n', because
   //they're the delimiters of getWord and getLine
   if ((str.find(' ') != string::npos) ||
       (str.find('\n') != string::npos))
   {
      throw invalid_argument("Can't seek multiple, delimited words");
   }

   streampos fileBackup;
   string lineBackup;
   string wordBackup;
   
   getPlace(fileBackup, lineBackup, wordBackup);
   
   while (word != str)
   {
      //End of stream
      if (file.eof() && line.eof())
      {
	 //Restore old place in the file
	 setPlace(fileBackup, lineBackup, wordBackup);

	 return false;
      }

      getWord();
   }

   //Eat the word you've found. NB not necessarily same line
   getWord();
   
   return true;
}

size_t pcdReader::readHeader()
{
   //Optional TODO actually check types, etc.
   //Not particularly useful at the moment.

   if (!seekWord("POINTS")) { throw invalid_argument("Invalid .pcd file: no 'POINTS' section"); }

   if (!word.size()) { throw invalid_argument("No surfels in .pcd file"); }

   size_t numSurfels = stoi(word);

   //TODO support 'DATA binary' files, too?

   seekWord("DATA");

   //Eat 'ascii', move onto next line
   getWord();
   
   return numSurfels;
}

vector<float> pcdReader::readBody(size_t numSurfels)
{
   vector<float> result;

   result.reserve(numSurfels * 4);

   const int numFields = 3; //xyz

   for (int i = 0; i < (int) numSurfels; ++i)
   {
      int j = 0;

      try //because stof() can throw
      {
	 for (; j < numFields; ++j)
	 {
	    getWordOnLine();

	    float wordBinary = stof(word);
	    
	    result.push_back(wordBinary);
	 }
      }

      //catch (const invalid_argument& inv)
      //catch (const out_of_range& ran)
      catch (const exception&)
      {
	 cerr << "Surfel row " << i << ", component " << j << " couldn't be read. Ignoring this surfel." << endl;

	 //Write over this surfel by popping from result
	 //NB since catch gotos, this current 'j' won't have
	 //pushed yet. So if j is 0, none to pop
	 for (int k = 0; k < j; ++k)
	 {
	    result.pop_back();
	 }

	 //Escape from below push_back
	 getLine();
	 continue;
      }

      //w
      result.push_back(1.0);

      getLine();
   }

   return move(result);
}

vector<float> pcdReader::read()
{
   vector<float> flts;
   
   size_t numSurfels = 0;
   
   try
   {
      numSurfels = readHeader();
   }

   catch (const exception& e)
   {
      cerr << e.what() << endl;
   }

   if (!numSurfels) { throw invalid_argument("Failure reading file header"); }
   
   flts = readBody(numSurfels);

   if (file.fail())
   {
      file.close();
      
      throw runtime_error("Failed to read all of the file");
   }

   file.close();

   return move(flts);
}

void sdlInstance::getError()
{
   //TODO make this an ostream thing 

   cerr << SDL_GetError(); // << endl;

   SDL_ClearError();
}

void sdlInstance::prep()
{
   if (SDL_Init(SDL_INIT_VIDEO) != 0)
   {
      cerr << "SDL failed to initialise: \'";

      getError();

      cerr << "\'" << endl;
   }

   //4.4 for glClearTex(Sub)Image
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);

   SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
		       SDL_GL_CONTEXT_PROFILE_CORE);

   uint32_t flags = SDL_WINDOW_OPENGL |
      SDL_WINDOW_RESIZABLE;

   window = SDL_CreateWindow("compute test",
			     
			     SDL_WINDOWPOS_CENTERED,
			     SDL_WINDOWPOS_CENTERED,
			     
			     SCRN_WIDTH,
			     SCRN_HEIGHT,
			     
			     flags);

   if (!window)
   {
      //TODO throw?

      cerr << "SDL failed to create window: \'";

      getError();

      cerr << "\'" << endl;
   }

   context = SDL_GL_CreateContext(window);

   if (!context)
   {
      cerr << "SDL failed to create OpenGL context: \'";

      getError();

      cerr << "\'" << endl;
   }

   if (SDL_GL_SetSwapInterval(1) == -1)
   {
      //TODO

      getError();
   }

   //Afaik this gives Glad the function by which it's to retrieve
   //pointers to specific GL functions, and makes it do so for all it
   //needs.
   //TODO figure out how to error check the SDL version, since this
   //is actually void...!
   if (!gladLoadGL())
   //if (!gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress))
   {
      //TODO throw, etc.
      
      cerr << "Glad failed to load some OpenGL function." << endl;
   }

   then = SDL_GetTicks();
   
   getError();
}

void sdlInstance::quit()
{
   SDL_DestroyWindow(window);
   SDL_GL_DeleteContext(context);

   SDL_Quit();
}

void sdlInstance::swapWindow()
{
   SDL_GL_SwapWindow(window);
}

void sdlInstance::pollEvents()
//General-purpose event registering
{
   now = SDL_GetTicks();

   while (SDL_PollEvent(&event))
   {
      switch (event.type)
      {
	 case SDL_QUIT:
	 { key_quit = true; }
	 
	 case SDL_KEYDOWN:
	 {
	    switch (event.key.keysym.sym)
	    {
	       case SDLK_ESCAPE:
	       { key_quit = true; break; }

	       case SDLK_w:
	       { key_w += now - event.key.timestamp; ++num_w; break; }
	       case SDLK_s:
	       { key_s += now - event.key.timestamp; ++num_s; break; }
	       case SDLK_a:
	       { key_a += now - event.key.timestamp; ++num_a; break; }
	       case SDLK_d:
	       { key_d += now - event.key.timestamp; ++num_d; break; }
	    }

	    break;
	 }

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
		  break;
	       }
	       case SDLK_s:
	       {
		  if (dt >= key_s) { key_s = 0; }
		  else { key_s -= dt; }
		  break;
	       }
	       case SDLK_a:
	       {
		  if (dt >= key_a) { key_a = 0; }
		  else { key_a -= dt; }
		  break;
	       }
	       case SDLK_d:
	       {
		  if (dt >= key_d) { key_d = 0; }
		  else { key_d -= dt; }
		  break;
	       }
	    }

	    break;
	 }
      }
   }

   then = now;
}

bool sdlInstance::getQuit() const { return key_quit; }

uint32_t sdlInstance::takeW()
{
   uint32_t result = key_w;

   key_w = 0;

   return result;
}

uint32_t sdlInstance::takeS()
{
   uint32_t result = key_w;

   key_s = 0;

   return result;
}

uint32_t sdlInstance::takeA()
{
   uint32_t result = key_a;

   key_a = 0;

   return result;
}

uint32_t sdlInstance::takeD()
{
   uint32_t result = key_d;

   key_d = 0;

   return result;
}

uint32_t sdlInstance::takeNumW() { uint32_t result = num_w; num_w = 0; return result; }
uint32_t sdlInstance::takeNumS() { uint32_t result = num_s; num_s = 0; return result; }
uint32_t sdlInstance::takeNumA() { uint32_t result = num_a; num_a = 0; return result; }
uint32_t sdlInstance::takeNumD() { uint32_t result = num_d; num_d = 0; return result; }

bool sdlInstance::getWindowSize(int& x, int& y)
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
   //variable (even after swapping) so do it once, outside of the function.
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

   bool needResize = (width > xy[0]) || (height > xy[1]);

   xy[0] = width; xy[1] = height;
   
   if (needResize)
   {
      glTexImage2D(GL_TEXTURE_2D,
		   0, //lod
		   GL_RGBA,
		   width,
		   height,
		   0, //border
		   GL_RGBA,
		   GL_UNSIGNED_BYTE,
		   nullptr);
   }

   else { pushSize(); }
}

void image::pushSize()
{
   //NB: uniforms have to be done while a shader (the right one) is being used.

   glUniform2uiv(xyLoc, 1, xy);

   LOG_GL();
}

void image::clear()
{
   /*
     It's fine to use 0 whether you use the texture as vec4(0, 0, 0,
     0) or uint(0), because it's unsigned.

     Unusually (?) RGBA reads 4 uints here. Just GL_RED puts alpha to 1.0!
   */
   
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

   //If you had version 4.4, BufferStorage would be better (TODO?)

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
   //and on the format of the buffer being a bunch of (32bit) vec4s)

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

static const uint32_t comp1LocalX = 64;
static const uint32_t comp1LocalY = 1;
static const uint32_t comp2LocalX = 1024;
static const uint32_t comp2LocalY = 1;

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

   //These aren't the max globals (ie # invocations). They're max -numbers- of workgroups.
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

void surfelModel::render()
{
   uint32_t xWkgps, yWkgps;
      
   getWkgpDimensions(xWkgps, yWkgps, comp1LocalX, comp1LocalY, getNumSurfels(), 1);

   glDispatchCompute(xWkgps,
		     yWkgps,
		     1);
}

float frustum::getFarDZ() const
{
   return nearDZ + planesDZ;
}

vec3 frustum::getDirX() const
{
   //By right-hand rule
   return cross(dirZ, dirY);
}

mat4 frustum::getInverseTransformMatrix() const
//Get a matrix factoring in the position and rotation of the camera.
//This must be inverse because you're not actually using it on the
//camera, but on all other objects; the camera can remain at
//origin. That way movement of the camera will be registered visibly
//in the movement of the other objects. (It's done this way because
//it's simpler to render things with the viewpoint at origin. For one
//thing you only need this one transform matrix rather than a
//camera-relative transform for every object.)
{
   vec3 inversePos = pos.inverse();

   //Don't inverse the directions. But you do need X
   vec3 dirX = getDirX();

   //I've inlined a matrix mult of the frustum's direction and
   //position here. They must be multiplied (rather than simply
   //putting the position in the translation slots of the matrix)
   //because the rotation is done in camera space, not in world space
   //(i.e. around the world origin).

   //You can ignore the 4th row/column of these matrices since they're
   //empty anyway.

   //These are the translation elements of the finished matrix.
   float nuPosX = dirX[0] * inversePos[0] +
      dirY[0] * inversePos[1] +
      dirZ[0] * inversePos[2];

   float nuPosY = dirX[1] * inversePos[0] +
      dirY[1] * inversePos[1] +
      dirZ[1] * inversePos[2];

   float nuPosZ = dirX[2] * inversePos[0] +
      dirY[2] * inversePos[1] +
      dirZ[2] * inversePos[2];

   return mat4(dirX[0], dirY[0], dirZ[0], 0.f,
	       dirX[1], dirY[1], dirZ[1], 0.f,
	       dirX[2], dirY[2], dirZ[2], 0.f,
	       nuPosX, nuPosY, nuPosZ, 1.f);

   #if 0
   return mat4(dirX[0], dirX[1], dirX[2], 0.f,
	       dirY[0], dirY[1], dirY[2], 0.f,
	       dirZ[0], dirZ[1], dirZ[2], 0.f,
	       nuPosX, nuPosY, nuPosZ, 1.f);

   return mat4(dirX[0], dirX[1], dirX[2], nuPosX,
	       dirY[0], dirY[1], dirY[2], nuPosY,
	       dirZ[0], dirZ[1], dirZ[2], nuPosZ,
	       0.f, 0.f, 0.f, 1.f);

   return (mat4(dirX[0], dirX[1], dirX[2], 0.f,
		dirY[0], dirY[1], dirY[2], 0.f,
		dirZ[0], dirZ[1], dirZ[2], 0.f,
		0.f, 0.f, 0.f, 1.f)
	   *
	   mat4(1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		inversePos[0], inversePos[1], inversePos[2], 1.f));
   #endif
}

mat4 frustum::getPerspectiveMatrix() const
{
   //Conversion to radians is in constructor now
   //Aspect ratio should be implicit
   float posX = tan(horFov);
   float posY = tan(verFov);

   //(planes + planes + near) / (planes + near - near)
   //= (2 * planes + near) / planes
   float zRow = 2 * nearDZ / planesDZ + 1;
//   float zRow = (2 * planesDZ + nearDZ) / planesDZ;
   // = near / planes + 2
   float wRow = -2.f * getFarDZ() * nearDZ / planesDZ;

   //TODO here or elsewhere in further matrix to be multiplied: figure
   //in the aspect ratio

   mat4 matrix = mat4(1.f / posX, 0.f, 0.f, 0.f,
		      0.f, 1.f / posY, 0.f, 0.f,
		      0.f, 0.f, zRow, 1.f,
		      0.f, 0.f, wRow, 0.f);
   
   return matrix;
}

vec3 frustum::getPos() const
{
   return pos;
}

void frustum::setPos(vec3 nuPos)
{
   pos = nuPos;
}

vec3 frustum::getZ() const
{
   return dirZ;
}

void camera::pushTransformMatrix()
{
   mat4 transf = getPerspectiveMatrix() * getInverseTransformMatrix();
//   mat4 transf = getPerspectiveMatrix();

   //+++++Test area++++++++
   if (!tested)
   {
      vec4 test = vec4(34.01294, 82.13033, 143.86603, 1.0);

      cout << "test = " << test[0] << ", " << test[1] << ", " << test[2] << endl;

      mat4 persp4 = getPerspectiveMatrix();
#if 0
      mat3 persp3 = mat3(persp4[0], persp4[1], persp4[2],
			 persp4[4], persp4[5], persp4[6],
			 persp4[8], persp4[9], persp4[10]);
#endif
      mat4 full4 = getPerspectiveMatrix() * getInverseTransformMatrix();
#if 0
      mat3 full3 = mat3(full4[0], full4[1], full4[2],
			full4[4], full4[5], full4[6],
			full4[8], full4[9], full4[10]);
#endif
      vec4 testPersp = persp4 * test;
      vec4 testFull = full4 * test;

      cout << "getPerspectiveMatrix() * getInverseTransformMatrix() = " << '\n' << 
	 full4[0] << ", " << full4[4] << ", " << full4[8] << ", " << full4[12] << '\n' <<
	 full4[1] << ", " << full4[5] << ", " << full4[9] << ", " << full4[13] << '\n' <<
	 full4[2] << ", " << full4[6] << ", " << full4[10] << ", " << full4[14] << '\n' <<
	 full4[3] << ", " << full4[7] << ", " << full4[11] << ", " << full4[15] << endl;

//      cout << "getPerspectiveMatrix() * test = " << testPersp[0] << ", " << testPersp[1] << ", " << testPersp[2] << endl;
      cout << "getPerspectiveMatrix() * getInverseTransformMatrix() * test = " << testFull[0] << ", " << testFull[1] << ", " << testFull[2] << endl;

      float ndcX = testFull[0] / testFull[3];
      float ndcY = testFull[1] / testFull[3];
      
      cout << "full ndc = " << ndcX << ", " << ndcY << endl;

      uint32_t halfX = 960 / 2;
      uint32_t halfY = 540 / 2;
      
      float screenX = (float) halfX * (1 + ndcX);
      float screenY = (float) halfY * (1 + ndcY);

      cout << "full screen xy = " << screenX << ", " << screenY << endl;

//      cout << "pos = " << pos[0] << ", " << pos[1] << ", " << pos[2] << '\n';
//      cout << "pos.inverse() = " << pos.inverse()[0] << ", " << pos.inverse()[1] << ", " << pos.inverse()[2] << endl;

      tested = true;
   }

   //++++++++++++++++++++++

   glUniformMatrix4fv(transformLoc,
		      1,
		      false,
		      (GLfloat*) &transf);
}

void camera::rotate(axisAngle aa)
{
   if (aa.angle() == 0.f) { return; }

   quaternion qu = quaternion(aa);

   dirY = qu.rotate(dirY);
   dirZ = qu.rotate(dirZ);

   //+++++++++++
//   cout << "dirY = " << dirY[0] << ", " << dirY[1] << ", " << dirY[2] << endl;
//   cout << "dirZ = " << dirZ[0] << ", " << dirZ[1] << ", " << dirZ[2] << endl;

//   cout << "Is dirY unit-length? " << dirY.isUnit(0.01) << endl;
//   cout << "Is dirZ unit-length? " << dirZ.isUnit(0.01) << endl;

   //Check for orthogonality?
   //+++++++++++
}

int main(int argc, char** args)
{
   SDL_SetMainReady();
   sdlInstance instance;

   program surfelsToSamples = program("compute.c.glsl");   
   program samplesToPixels = program("compute2.c.glsl");

   LOG_GL();

   //Programs have to be used while uniforms are loaded
   surfelsToSamples.use();
   //The first 2 arguments are ad hoc bindings, from the shaders - for wid/height
   image samples = image(3);
   
   LOG_GL();

   samplesToPixels.use();
   image pixels = image(5);

   LOG_GL();

   surfelsToSamples.use();
   samples.prep(SCRN_WIDTH * 2, SCRN_HEIGHT * 2);

   samplesToPixels.use();
   pixels.prep(SCRN_WIDTH, SCRN_HEIGHT);

   LOG_GL();

   //Bind images to texture units
   samples.use(1, GL_READ_WRITE);
   pixels.use(2, GL_WRITE_ONLY);

   LOG_GL();

   surfelModel surfels;

   LOG_GL();

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
   framebuffer frame;

   LOG_GL();
   
   frame.prep(pixels);
   
   LOG_GL();

   frame.use();

   LOG_GL();

   //Uniform stuff for the first time (e.g. perspective matrix)
   GLint perspectiveLoc = glGetUniformLocation(surfelsToSamples.getHandle(),
					       "perspective");

   LOG_GL();

   camera cam = camera(perspectiveLoc,
		       vec3(-69.0, 2.456, 91.0),
		       vec3(0.0, 0.0, 1.0), //dirZ
		       vec3(1.0, 0.0, 0.0), //dirY
		       65.f, //horizontal fov
		       pixels.getAspectRatio(), //aspect ratio
		       1.f, 10.f); //irrelevant until you clip

   LOG_GL();

   surfelsToSamples.use();
   cam.pushTransformMatrix();

   LOG_GL();
   
   int winX, winY;

   LOG_GL();

   while (!instance.getQuit())
   {
      bool cameraMoved = false;

      instance.pollEvents();

//      const float speed = 0.00000001;
      const float speed = 5.f;
      const float rotSpeed = 0.000000001;
      
      if (uint32_t numW = instance.takeNumW())
      {
	 vec3 pos = cam.getPos();

	 pos = pos + cam.getZ() * (float) numW * speed;

	 cam.setPos(pos);

//	 cout << pos[0] << ", " << pos[1] << ", " << pos[2] << endl;

	 cameraMoved = true;
      }

      if (uint32_t numS = instance.takeNumS())
      {
	 vec3 pos = cam.getPos();

	 pos = pos - cam.getZ() * (float) numS * speed;

	 cam.setPos(pos);

//	 cout << pos[0] << ", " << pos[1] << ", " << pos[2] << endl;

	 cameraMoved = true;
      }

      const float angle = 5.f;

      if (uint32_t numA = instance.takeNumA())
      {
//	 cam.rotate(axisAngle(vec3(0.f, 1.f, 0.f), -rotSpeed * (float)
//	 a));
	 vec3 axisRot = vec3(0.f, 1.f, 0.f);
	 
	 cam.rotate(axisAngle(axisRot, -angle * numA));

//	 cout << "Rotating by " << -rotSpeed * (float) a << " degrees"
//	 << endl;
	 cout << "Rotating by " << -angle * numA << endl;

	 cameraMoved = true;
      }

      if (uint32_t numD = instance.takeNumD())
      {
	 //cam.rotate(axisAngle(vec3(0.f, 1.f, 0.f), rotSpeed *
	 //(float) d));
	 cam.rotate(axisAngle(vec3(0.f, 1.f, 0.f), angle * numD));

//	 cout << "Rotating by " << rotSpeed * (float) d << " degrees"
//	 << endl;
	 cout << "Rotating by " << angle * numD << endl;

	 cameraMoved = true;
      }

      //If the window's size has changed, size of buffers must change
      //with it.      
      if (instance.getWindowSize(winX, winY))
      {
	 //These also do associated uniforms
	 samples.resize(winX * 2, winY * 2);
	 pixels.resize(winX, winY);

	 //Perspective matrix will need re-uniforming since it
	 //will have aspect ratio baked in
	 cameraMoved = true;

	 //TODO: may have to change framebuffer relationship
	 //too. Currently resizing doesn't update samplesXY correctly
      }

      surfelsToSamples.use();

      if (cameraMoved) { cam.pushTransformMatrix(); }

      LOG_GL();

      surfels.render();

      LOG_GL();

      glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

      samplesToPixels.use();

      LOG_GL();

      //TODO check workgroup maximums
      uint32_t xWkgps, yWkgps;
      
      getWkgpDimensions(xWkgps, yWkgps, comp2LocalX, comp2LocalY, winX, winY);

      glDispatchCompute(xWkgps,
			yWkgps,
			1);

      LOG_GL();

      glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

      pixels.blit(frame);

      glFlush(); //Executes any lazy command buffers
      glFinish(); //Blocks til they're done

      instance.swapWindow();

      LOG_GL();

      samples.clear();
      pixels.clear();

      LOG_GL();

      //TODO may have to reattach pixels, since change in fb...?
      
      glClear(GL_COLOR_BUFFER_BIT);

      LOG_GL();
   }

   printErrorsGL();

   return 0;
}
