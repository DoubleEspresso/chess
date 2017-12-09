#pragma once

#ifndef SBC_UCI_H
#define SBC_UCI_H

#include <vector>
#include <string>
#include <sstream>

#include "board.h"

namespace UCI
{
  void loop();
  void command(std::string cmd, int& GAME_OVER);
  void load_position(std::string& pos);
  void move_from_string(std::string& move);
  std::string move_to_string(U16& m);
  U16 get_move(std::string& move);
  void analyze(Board& b);
};

#endif

