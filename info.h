#pragma once

#ifndef SBC_INFO_H
#define SBC_INFO_H

#include <stdio.h>
#include <time.h>
#include <new>
#include <string>

#include "definitions.h"

namespace Info
{
  namespace BuildInfo
  {
    void greeting();
  }
  namespace Date
  {
    bool today(char tbuff[], size_t sz);
  }
  namespace GpuSupport
  {

  }
}

void Info::BuildInfo::greeting()
{
  char mode[256];
  if (sizeof(size_t) == 4) {
#ifdef DEBUG
    sprintf(mode, "32-bit (dbg)");
#else
    sprintf(mode, "32-bit");
#endif
  }
  else {      
#ifdef DEBUG
    sprintf(mode, "64-bit (dbg)");
#else
    sprintf(mode, "64-bit");
#endif
  }
  printf("%s %s by %s\n", ENGINE_NAME, mode, AUTHOR);
}

bool Info::Date::today(char tbuff[], size_t sz)
{
  try {
    time_t now = time(0);
    struct tm * tinfo = new tm();
    
#ifdef _WIN32		
    localtime_s(tinfo, &now);
#else
    tinfo = localtime(&now);
#endif
    strftime(tbuff, sz, "%m/%d/%Y %X", tinfo);
  }
  catch (const std::exception& e) {
    printf("..[Info] Date::today() exception %s\n", e.what());
    return false;
  }
  return true;
}


#endif
