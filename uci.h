#pragma once

#ifndef HEDWIG_UCI_H
#define HEDWIG_UCI_H

#include <vector>
#include <string>
#include <sstream>

#include "board.h"

namespace UCI
{
  void cmd_loop();
  void uci_command(std::string cmd, int& GAME_OVER);
  //void PrintBest(Board b);
  void load_position(std::string& pos);
  void move_from_string(std::string& move);
  //void run_testing(int fixed_time, int fixed_depth);
  std::string move_to_string(U16& m);
  U16 get_move(std::string& move);
  void analyze(Board& b);
  //std::string move_to_san(U16& m);
};

// for testing / debugging 
typedef struct
{
  std::string fen;
  std::string bestmove;
  std::string scores;
} TestPosition;

typedef struct
{
  std::string desc;
  std::vector<TestPosition> tests;

  // stats
  float total;
  float mean;
} EPDTest;



//extern std::vector<EPDTest> TestSuite;

#endif

