#pragma once
#ifndef SBC_SEARCH_H
#define SBC_SEARCH_H

#ifdef WIN32
#define NOMINMAX
#endif

#include <vector>
#include <algorithm>

#include "definitions.h"
#include "globals.h"
#include "utils.h"
#include "board.h"


enum NodeType { ROOT, PV, CUT, SPLIT, NONPV, NONE };

class ThreadWorker;
class MoveSelect;
class SplitBlock;

struct Score {
  int beta;
  int eval;
};

struct Limits {
  int wtime;
  int btime;
  int winc;
  int binc;
  int movestogo;
  int nodes;
  int movetime;
  int mate;
  int depth;
  bool infinite;
  bool ponder;
};


struct Node {
  NodeType type;
  U16 currmove, bestmove , threat, excludedMove;
  U16 killer[4];
  int ply, static_eval, threat_gain;
  bool isNullSearch;
  bool givescheck;
  U16 pv[MAXDEPTH];
  SplitBlock * sb;
};

struct SignalsType {
  bool stop, stopOnPonderhit, firstRootMove, failedLowAtRoot, timesUp;
};

namespace Search {
  // alpha-beta search
  extern void run(Board& b, int dpth, bool pondering=false);
  extern void from_thread(Board& b, int alpha, int beta, int depth, Node& node);
};

// global data
extern std::vector<U16> RootMoves;
extern SignalsType UCI_SIGNALS;

#endif
