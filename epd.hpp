/*
-----------------------------------------------------------------------------
This source file is part of the Havoc chess engine
Copyright (c) 2020 Minniesoft
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
#pragma once


#ifndef EPD_H
#define EPD_H


#include <string>
#include <iostream>
#include <fstream>
#include <cassert>
#include <vector>
#include <sstream>


#include "pgn.h"
#include "position.h"
#include "uci.h"
#include "utils.h"

struct epd_entry {
  std::string pos;
  std::string bestmove;
};

class epd {

  std::vector<epd_entry> positions;
  bool load(const std::string& filename);

public:
  epd() {}
  epd(const std::string& filename) { load(filename);  }
  epd(const epd& o) = delete;
  epd(const epd&& o) = delete;
  ~epd() {}

  epd& operator=(const epd& o) = delete;
  epd& operator=(const epd&& o) = delete;

  inline std::vector<epd_entry> get_positions() { return positions; }
};



bool epd::load(const std::string& filename) {
  // assuming every line of the epd file is a string..
  std::string line("");
  std::ifstream epd_file(filename);
  size_t counter = 0;
  while (std::getline(epd_file, line)) {

    epd_entry e;
    position p;
    pgn _pgn;

    std::vector<std::string> tokens = util::split(line, ';');

    if (tokens.size() <= 0) {
      std::cout << "skipping invalid line: " << line << std::endl;
    }

    // skip the avoid move positions 

    // the token of interest is the first token
    std::string epd_line = tokens[0];


    if (epd_line.find("bm") == std::string::npos) {
      std::cout << "..skipping invalid line: " << epd_line << std::endl;
      continue;
    }


    e.pos = epd_line.substr(0, epd_line.find(" bm "));
    e.bestmove = epd_line.substr(epd_line.find("bm ") + 3); // to the end

    //std::cout << "dbg: pos: " << e.pos << " bm: " << e.bestmove << std::endl;

    // conver SAN -> from-to move
    std::istringstream pstream(e.pos);
    p.setup(pstream);
    Move m;
    _pgn.move_from_san(p, e.bestmove, m);
    e.bestmove = uci::move_to_string(m);
    positions.push_back(e);

    //std::cout << "  dbg: parsed bm: " << e.bestmove << std::endl;

    ++counter;
  }
  std::cout << "loaded " << counter << " test positions" << std::endl;

  return true;
}

#endif