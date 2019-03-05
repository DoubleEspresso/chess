#ifndef SEARCH_H
#define SEARCH_H

#include <algorithm>

#include "types.h"
#include "threads.h"
#include "move.h"
#include "uci.h"



namespace Search {
  
  std::atomic_bool searching;  
  std::mutex mtx;
  Move bestmoves[2];
  
  struct node {
    U16 ply;
    bool in_check, null_search, gen_checks;
    Move curr_move, best_move, threat_move;
    Move deferred_moves[218];
    Move killers[4];
    Score static_eval;
  };

  void search_timer(position& p, limits& lims);
  void start(position& p, limits& lims);
  void iterative_deepening(position& p, U16 depth);
  void readout_pv(position& p, const Score& eval, const U16& depth);
  double estimate_max_time(position& p, limits& lims);

  template<Nodetype type>
  Score search(position& p, int16 alpha, int16 beta, U16 depth, node * stack);

  template<Nodetype type>
  Score qsearch(position& p, int16 alpha, int16 beta, U16 depth, node * stack);
}


#include "search.hpp"

#endif
