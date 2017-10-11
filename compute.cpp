#include <glad/glad.h>
#include "compute.hpp"
#include <iostream>
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

void pcdReader::prep(const string filename)
{
   if ((filename[filename.size() - 4] != '.') ||
       (filename[filename.size() - 3] != 'p') ||
       (filename[filename.size() - 2] != 'c') ||
       (filename[filename.size() - 1] != 'd'))
   { throw invalid_argument("File name given, \"" + filename + "\", was not a .pcd"); }

   file.open(filename, ifstream::in);

   if (file.fail())
   {
      file.close();
      
      throw invalid_argument("No such .pcd file \"" + filename + "\", or problem reading file");
   }

   getLine();
   getWord();

   #if 0
   getChar();
   #endif
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

#if 0
char pcdReader::getChar()
{
   return cur = file.get();
}
#endif

#if 0
string pcdReader::getWord()
{
   string word;

   //Power through until you hit a character   
   while ((cur == ' ') || (cur == '\n'))
   {
      if (cur == char_traits<char>::eof()) { return word; }

      getChar();
   }

   while (cur != char_traits<char>::eof())
   {
      bool whitespace = (cur == ' ') || (cur == '\n');

      if (!whitespace) { word.push_back(cur); }

      else break;
   }

   return move(word);
}
#endif

#if 0
bool pcdReader::seekStr(string str)
//Specifically, seek til -after- the string
{
   while (true)
   {
      bool abort = false;

      for (unsigned int i = 0; i < str.size(); ++i)
      {
	 if (cur == char_traits<char>::eof()) { return false; }

	 if (cur != str[i])
	 {
	    break;
	 }

	 //This includes the last letter of the string (it should do).
	 getChar();
      }

      //If the whole sequence succeeded, you're there
      if (!abort) { return true; }
   }

   return true;
}
#endif

size_t pcdReader::readHeader()
{
#if 0
   if (!seekWord("FIELDS")) { throw invalid_argument("Invalid .pcd file: no 'FIELDS' section"); }

   //See whether colour/normals are included; they will be ignored

   numFields = 0;
   
   while (word.size())
   {
      ++numFields;

      getWordOnLine(); //TODO this loops; change
   }
#endif
   
   #if 0
   //ie, within this line...
   while (cur != '\n')
   {
      if (cur == char_traits<char>::eof()) { return 0; }

      //Checking # of strings delimited by whitespace
      if (cur != ' ')
      {
	 ++numFields;
	 
	 while (cur != ' ')
	 {
	    if (cur == '\n') { break; }

	    getChar();

	    if (cur == char_traits<char>::eof()) { return 0; }
	 }
      }
   }
   #endif

   //Optional TODO actually check types, etc.
   //Not particularly useful at the moment.

   size_t numSurfels = 0;

   if (!seekWord("POINTS")) { throw invalid_argument("Invalid .pcd file: no 'POINTS' section"); }

   if (!word.size()) { throw invalid_argument("No surfels in .pcd file"); }

   numSurfels = stoi(word);

   #if 0
   string pts;

   while (cur != '\n')
   {
      if (cur == char_traits<char>::eof()) { return 0; }

      pts.push_back(cur);

      getChar();
   }

   numSurfels = stoi(pts);
   #endif

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

   //This is artificial and temporary
   numFields = 3;

   for (unsigned int i = 0; i < numSurfels; ++i)
   {
      for (unsigned int j = 0; j < numFields; ++j)
      {
	 result.push_back(stof(getWordOnLine()));
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
      
      cout << "Glad failed to load some OpenGL function." << endl;
   }

   //TODO change
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

bool sdlInstance::checkQuit()
{
   //This function won't last very long; it compresses an event loop
   //into just checking for quits.

   while (SDL_PollEvent(&event))
   {
      switch (event.type)
      {
	 case SDL_QUIT:
	 { return true; }
	 
	 case SDL_KEYDOWN:
	 {
	    switch (event.key.keysym.sym)
	    {
	       case SDLK_ESCAPE:
	       { return true; }

	       default:
	       { break; }
	    }

	    break;
	 }

	 default:
	 { break; }
      }
   }

   return false;
}

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

	 string error = getErrorGL();

	 if (error.size()) { cerr << ": " << error << endl; }
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

      string error = getErrorGL();

      if (error.size()) { cerr << error << endl; }

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

bool framebuffer::prep(image& img)
{
   glGenFramebuffers(1, &handle);
   glBindFramebuffer(GL_FRAMEBUFFER, handle);

   glFramebufferTexture2D(GL_FRAMEBUFFER,
			  GL_COLOR_ATTACHMENT0,
			  GL_TEXTURE_2D,
			  img.getHandle(),
			  0);

   //TODO check

   return true;
}

void framebuffer::quit()
{
   glDeleteFramebuffers(1, &handle);
}

void image::prep(GLsizei width, GLsizei height)
{
   glGenTextures(1, &handle);
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

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   //TODO checks

   //Size uniforms
   x = width;
   y = height;

   //Locations: don't bother with glGetUniformLocation, that'd require
   //a string. Just use a layout() qualifier in the shader.
   //(So xLoc and yLoc will be part of the constructor.)
   
   pushSize();
}

void image::quit()
{
   glDeleteTextures(1, &handle);
}

void image::use(GLuint binding)
{
   glBindImageTexture(binding,
		      handle,
		      0, //level
		      GL_FALSE, //layered
		      0, //layer
		      GL_READ_WRITE, //Clearly, could have a different class
		      GL_RGBA32UI);
}

void image::resize(GLsizei width, GLsizei height)
{
   /*
     Overcomplicated method of reducing time spent resizing:
     If the requirements -shrink-, then no need to reallocate entire
     image; can just consistently use SubBuffer.

     NB This precludes using imageSize in a shader. There must be
     uniforms with the size, instead. Hence pushSize().
   */

   bool needResize = (width > x) || (height > y);

   x = width;
   y = height;
   
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
   glUniform1ui(xLoc, x);
   glUniform1ui(yLoc, y);
}

void image::clear()
{   
   uint32_t val = 0; //Assuming atomicMax not min

   glClearTexSubImage(handle,
		      0, //level
		      0, 0, 0, //origin x,y,z
		      x, y, 1, //1: depth
		      GL_RGBA,
		      GL_UNSIGNED_INT,
		      &val);
}

void buffer::prep(vector<float> data, GLuint binding)
{
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

   vector<float> load;

   try
   {
      load = pcd.read();
   }

   catch (const exception e) { cerr << e.what() << endl; }

   if (load.size() % (4 * sizeof(uint32_t))) { throw invalid_argument("Surfel input data not divisible into groups of 4 floats"); }

   data.prep(load, binding);
}

size_t surfelModel::getNumSurfels() const
{
   return data.size() / 4;
}

static const uint32_t comp1ThreadsPerWkgp = 1;
static const uint32_t comp2ThreadsPerWkgp = 90 * 90;

void getWkgpDimensions(uint32_t& x, uint32_t& y,
		       uint32_t threadsPerWkgp,
		       uint32_t threads)
{
   const float numWkgps = threads / threadsPerWkgp;

   const uint32_t floor = sqrt(numWkgps);

   const uint32_t remainder = numWkgps - floor;

   y = floor;
   x = floor + remainder / y + 1;
}

void surfelModel::render()
{
   uint32_t xWkgps, yWkgps;

   getWkgpDimensions(xWkgps, yWkgps, comp1ThreadsPerWkgp, getNumSurfels());

   glDispatchCompute(xWkgps,
		     yWkgps,
		     1);
}

int main(int argc, char** args)
{
   SDL_SetMainReady();
   sdlInstance instance;

   program surfelsToSamples = program("compute.c.glsl");
   
   program samplesToPixels = program("compute2.c.glsl");

   //The arguments are ad hoc bindings, from the shaders
   image samples = image(3, 4);
   image pixels = image(5, 6);

   pixels.prep(SCRN_WIDTH, SCRN_HEIGHT);
   samples.prep(SCRN_WIDTH * 2, SCRN_HEIGHT * 2);

   surfelModel surfels;

   const GLuint surfelsBinding = 1;
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

   //TODO: framebuffer stuff? shader needs to write
   //directly/indirectly

   //TODO: clear stuff first time, without using instance.swapWindow()

   int winX, winY;

   while (!instance.checkQuit())
   {
      //If the window's size has changed, size of buffers must change
      //with it.      
      if (instance.getWindowSize(winX, winY))
      {
	 samples.resize(winX * 2, winY * 2);
	 pixels.resize(winX, winY);

	 //TODO might also have to change relationship between pixels
	 //and framebuffer (though not mentioned by
	 //glFramebufferTexture afaik)
	 //Note that if you don't - and shaders use imageSize to
	 //figure out indexing - then they will be accessing garbage
      }

      surfelsToSamples.use();

      surfels.render();
      
      //Primitive inter-pipeline barrier
      glFlush();
      glFinish();

      //TODO Better to put a barrier in the following shader?

      samplesToPixels.use();

      uint32_t xWkgps, yWkgps;

      getWkgpDimensions(xWkgps, yWkgps, comp2ThreadsPerWkgp, winX * winY);

      glDispatchCompute(xWkgps,
			yWkgps,
			1);

      glFlush(); //Executes any lazy command buffers
      glFinish(); //Blocks til they're done

      instance.swapWindow();

      samples.clear();
      pixels.clear();

      //TODO may have to reattach pixels, since change in fb...?
      
      glClear(GL_COLOR_BUFFER_BIT);
   }

   return 0;
}
