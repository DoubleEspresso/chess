#include <cstring>
#include <iostream>
#include <functional>
#include <stdio.h>
#include <stdlib.h>

#include "opts.h"
#include "definitions.h"

// global options
Options options;

Options::Options() : loaded(false)
{
  if (!load())
    load_defaults();

  loaded = true;
}

Options::~Options()
{

}

bool Options::load()
{
  std::ifstream settings("sbchess.ini");
  if (!settings) 
    {
      //dbg(Log::write("..[Settings] Error loading settings file\n"););
      return false;
    } 
  //else printf("\n init options...\n");
  std::string line, key, val;
  
  while (getline(settings, line))
    {
      if (line[0] == '[') continue;
      std::string tmp = ""; unsigned int idx = 0;
      
      do { if (idx - 1 == line.length()) break; } while ( line[idx++] != '=' ); --idx;
      if (idx == 0) continue;
      key = line.substr(0,idx); val = line.substr(idx+1, line.length()-1);      
      
#ifdef _WIN32
      optionMap.insert(std::pair<std::string, int>(key, std::atof(val.c_str())));
#else
      optionMap.insert(std::pair<std::string, int>(key, std::stof(val)));
#endif
      //printf("..%s = %f\n", key.c_str(), std::atof(val.c_str()));      
    }
  settings.close();  
  loaded = true;

  return true;
}

void Options::show()
{
  printf("..current options ..\n");
  for( opt_it it = optionMap.begin(); it != optionMap.end(); ++it )
    {
      std::string sk(it->first); float val = 0;
      val = float(it->second);
      std::cout << sk << "=" << val << std::endl;
    }
}

void Options::load_defaults()
{
#ifdef _WIN32
#define mp(x,y) optionMap.insert(std::make_pair(x, y))
#else
#define mp(x,y) optionMap.insert(std::pair<std::string, int>(x, y))
#endif

  // tablebases (opening/endgame)
  // nb opening book locations not set in settings, assumed same dir as engine exe
  mp("opening book", false); 
  mp("ending book", false);

  // hash table params
  mp("hash table size mb", 100);
  mp("material table size mb", 100);
  mp("pawn table size mb", 100);

  // search params
  mp("razor margin", 650);
  mp("futility scaling", 200);
  mp("null move reduction", 2);
  mp("futility margin", 650);
  mp("late move reduction margin", 650);
  mp("history prune cut", -1001);

  // material parameters
  mp("pawn value mg", 100);
  mp("pawn value eg", 110);
  mp("knight value mg", 330);
  mp("knight value eg", 345);
  mp("bishop value mg", 360);
  mp("bishop value eg", 385);
  mp("rook value mg", 550);
  mp("rook value eg", 575);
  mp("queen value mg", 990);
  mp("queen value eg", 1015);
  mp("bishop pair weight mg", 16);
  mp("bishop pair weight eg", 20);
  mp("rook redundancy weight mg", 3);
  mp("rook redundancy weight eg", 1.15);
  mp("queen redundancy weight mg", 0.5);
  mp("queen redundance weight eg", 0.25);

  // pawn evaluation params
  mp("doubled pawn penalty mg",-4.0);
  mp("doubled pawn penalty eg",-8.0);
  mp("shelter pawn penalty mg",4.0);
  mp("shelter pawn penalty eg",1.0);
  mp("isolated pawn penalty mg",-10.0);
  mp("isolated pawn penalty eg",-20.0);
  mp("backward pawn penalty mg",-5.0);
  mp("backward pawn penalty eg",-1.0);
  mp("chain pawn penalty mg",4.0);
  mp("chain pawn penalty eg",8.0);
  mp("passed pawn penalty mg",10.0);
  mp("passed pawn penalty eg",20.0);

  // main eval params
  mp("trace eval",0);
  mp("tempo bonus",16);
  mp("pawn attack scaling",1);
  mp("knight attack scaling",1);
  mp("bishop attack scaling",1);
  mp("rook attack scaling",1);
  mp("queen attack scaling",1);

  // move ordering params
  mp("counter move bonus", 25);

  // square scaling params
  mp("pawn scaling", 1);
  mp("knight scaling", 1);
  mp("bishop scaling", 1);
  mp("rook scaling", 1);
  mp("queen scaling", 1);
#undef mp
}
	  
/*
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
	  float val = opts.second();
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
*/
