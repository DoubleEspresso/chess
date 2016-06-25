#ifndef HEDWIG_OPTIONS_H
#define HEDWIG_OPTIONS_H

#ifdef _WIN32
  #include <utility>
#endif
#include <map>
#include <fstream>
#include <new>
#include <sstream>
#include <string>
#include <iostream>

#include "threads.h"

class Options
{
public:
  Options();
  ~Options();
  bool load();
  void load_defaults();
  bool close();
  void show();
  typedef std::map<std::string, float>::iterator opt_it;
  //void begin() { omap_it = optionMap.begin(); }
  //bool end() { return omap_it == optionMap.end(); }
  //const char* first() {return omap_it->first;}
  //int second() {return omap_it->second;}
  //opt_it& operator++()
  //{
  //return ++omap_it; 
  //}

  inline float operator[](std::string s)
  {
    mutex.lock();
    float val = 0;
    for( opt_it it = optionMap.begin(); it != optionMap.end(); ++it )
    {
      std::string sk(it->first);
      if (sk == s) 
	{
	  val = float(it->second);
	}
    }
    mutex.unlock();
    return val;
   }

  private:
  NativeMutex mutex;
  bool loaded;
  std::map<std::string, float> optionMap;
};

extern Options options; // global opts
#endif

