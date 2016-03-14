#pragma once

#ifndef HEDWIG_UCI_H
#define HEDWIG_UCI_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>


#include "globals.h"
//#include "move.h"
//#include "pawns.h" //testing
//#include "evaluate.h" // testing
//#include "search.h" //testing
#include "utils.h"

namespace UCI
{
	void cmd_loop();
	void uci_command(std::string cmd, int& GAME_OVER);
	//void PrintBest(Board b);
	void load_position(std::string& pos);
	void move_from_string(std::string& move);
	//void run_testing(int fixed_time, int fixed_depth);
	std::string move_to_string(U16& m);
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

