#ifndef SEARCH_H
#define SEARCH_H

#include <algorithm>

#include "types.h"
#include "threads.h"
#include "move.h"




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

  void search_timer();
  void start(position& p, U16 depth);
  void iterative_deepening(position& p, U16 depth);
  void readout_pv(position& p, const Score& eval, const U16& depth);
  
  template<Nodetype type>
  Score search(position& p, int16 alpha, int16 beta, U16 depth, node * stack);

  template<Nodetype type>
  Score qsearch(position& p, int16 alpha, int16 beta, U16 depth, node * stack);
}


#include "search.hpp"

#endif
