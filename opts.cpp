#include <cstring>
#include <iostream>
#include <functional>
#include <stdio.h>
#include <stdlib.h>

#include "opts.h"
#include "definitions.h"

// global options
Options opts;

Options::Options() : loaded(false)
{

}

Options::~Options()
{

}

bool Options::load()
{
  try 
    {
      std::ifstream settings("hedwig.ini");
      if (!settings) 
	{
	  //dbg(Log::write("..[Settings] Error loading settings file\n"););
	  return false;
	} 
      else printf("\n init options...\n");
      std::string line, key, val;
      
      while (getline(settings, line))
	{
	  if (line[0] == '[') continue;
	  std::string tmp = ""; unsigned int idx = 0;
	  
	  do { if (idx - 1 == line.length()) break; } while ( line[idx++] != '=' ); --idx;
	  if (idx == 0) continue;
	  key = line.substr(0,idx); val = line.substr(idx+1, line.length()-1);
	  
	  
#ifdef _WIN32
	  optionMap.insert(std::make_pair(ckey, std::atof(val.c_str())));
#else
	  optionMap.insert(std::make_pair<const char*, int>(key.c_str(), std::atof(val.c_str())));
#endif
	  printf("..%s = %f\n", key.c_str(), std::atof(val.c_str()));      
	}
	settings.close();
	loaded = true;
    }
    catch(const std::exception& e) 
      {
	  //dbg(printf("..[Options] exception : Options::load() %s\n",e.what()));
	  loaded = false;
	  return false;
      }
    return true;
 }
	  
 bool Options::close()
 {
   try 
     {
       std::ofstream settings("settings",  std::ofstream::trunc);
       if (!settings) 
	 {
	  //dbg(Log::write("..[Options] Error opening settings file for writing\n"););
	  return false;
	  }
       for(opts.begin(); !opts.end(); ++opts) 
	 {
	   std::string key(opts.first());
	   int val = opts.second();
	   settings << key << "\t" << val << "\n";
	  }
       settings.close();
     }
   catch(const std::exception& e) 
     {
       printf("..[Options] exception : Options::close() %s\n", e.what());
       return false;
     }
   return true;
}
