#pragma once
#ifndef SBC_SEARCH_H
#define SBC_SEARCH_H

#ifdef WIN32
#define NOMINMAX
#endif

#include <vector>
#include <algorithm>
#include <fstream>

#include "definitions.h"
#include "globals.h"
#include "utils.h"
#include "board.h"
#include "uci.h"
#include "move.h"
#include "hashtable.h"

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

struct NodeInfo {
  U64 id;
  int alpha;
  int beta;
  int eval;
  U16 move;
  int stm;
  TableEntry * e;
  int depth;
  NodeType type;
  bool hash_hit;
  bool razor_prune;
  bool fwd_futility_prune;
  bool main_futility_prune;
  bool null_mv_prune;
  bool iid_hit;
  bool see_prune;
  bool beta_prune;
  bool fail_low;
  bool check_extension;
  int lmr;
  int reduction;
  bool fulldepthsearch;
  
  void move_info(std::ofstream& ofile) {
    ofile << id << ": "
	  << "move=" << UCI::move_to_string(move) << "\n";
  }

  void eval_info(std::ofstream& ofile) {
    ofile << id << ": "
	  << "(alpha,eval,beta)="
	  << alpha << "," << eval << "," << beta << "\n";
  }

  void tt_info(std::ofstream& ofile) {
    if (hash_hit && e) {
      ofile << id << ": "
	    << "hash hit found "
	    << "e.move=" << UCI::move_to_string(e->move) << " "
	    << "e.value=" << e->value << " "
	    << "static_value=" << e->static_value << " "
	    << "e.depth=" << e->Depth() << " "
	    << "e.bound=" << (e->bound == BOUND_EXACT ? "exact" : e->bound == BOUND_LOW ? "low" : "high") << "\n";      
    }
    else {
      ofile << id << ": "
	    << "no hash hit found" << "\n";
    }
  }

  void search_info(std::ofstream& ofile) {
    ofile << id << ": "
          << "type=" << (type == ROOT ? "root" : type == PV ? "pv" : "non-pv") << " "
	  << "reduction=" << reduction << " "
	  << "LMR=" << lmr << " "
	  << "fulldepthsearch=" << (fulldepthsearch ? "yes" : "no") << " "
	  << "check extension=" << (check_extension ? "yes" : "no") << " "
	  << "fail low=" << (fail_low ? "yes" : "no") << "\n"; 
  }
  
  
  void prune_info(std::ofstream& ofile) {
    ofile << id << ": prune info="
	  << (razor_prune ? "razor" :
	      fwd_futility_prune ? "fwd futility" :
	      main_futility_prune ? "main futility" :
	      null_mv_prune ? "null prune" :
	      beta_prune ? "beta cut prune" :
	      see_prune ? "SEE prune" : "none") << "\n";
      }
    

  void log_search_start(std::ofstream& ofile) {
    ofile << id << ": ENTERS PVS: move=" << UCI::move_to_string(move) << " depth=" << depth << " stm=" << (stm == WHITE ? "white" : "black") << "\n";
    eval_info(ofile);
    tt_info(ofile);
  }
  
  void log_search_end(std::ofstream& ofile) {
    ofile << id << ": EXITS PVS\n";
    eval_info(ofile);
    move_info(ofile);
    prune_info(ofile);
    search_info(ofile);
    tt_info(ofile);
  }
  
};

struct Node {
  NodeInfo ni;
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
  extern float mc_rollout(Board& b);
}

// global data
extern std::vector<MoveList> RootMoves;
extern SignalsType UCI_SIGNALS;

#endif
