#pragma once

#ifndef INFO_H
#define INFO_H

#include <stdio.h>
#include <time.h>
#include <new>
#include <string>

#define ENGINE_NAME "Sanbox Chess"
#define AUTHOR "M.Glatzmaier"
#define VERSION "0.0.0"

namespace {
  
  void greeting() {
    bool dbg = false;
#ifdef DEBUG
    dbg = true;
#endif
    std::string bits = (sizeof(size_t) == 4 ? "32-bit" : "64-bit");
    std::string debug = (dbg ? "debug" : "");
    std::cout << ENGINE_NAME  << " " << bits << " by "  << AUTHOR << std::endl;
  }  
}

#endif
