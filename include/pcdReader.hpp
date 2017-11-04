#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <sstream>

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
