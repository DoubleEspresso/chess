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

class Options
{
public:
  Options();
  ~Options();
  bool load();
  bool close();
  
  typedef std::map<const char*, float>::iterator opt_it;
  void begin() { omap_it = optionMap.begin(); }
  bool end() { return omap_it == optionMap.end(); }
  const char* first() {return omap_it->first;}
  int second() {return omap_it->second;}
  opt_it& operator++()
  {
    return ++omap_it; 
  }
  
  inline float operator[](std::string s)
  {
    if (!loaded) load();
    int val = 0;
    for( opt_it it = optionMap.begin(); it != optionMap.end(); ++it )
    {
      std::string sk(it->first);
      if (sk == s) 
        val = it->second;
    }
    return val;
   }

  private:
    std::map<const char*, float> optionMap;
    bool loaded;
    opt_it omap_it;
};

extern Options opts; // global opts
#endif

