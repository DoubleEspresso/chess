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
  void start(position& p, limits& lims, bool silent);
  void iterative_deepening(position& p, U16 depth, bool silent);
  void readout_pv(position& p, const Score& eval, const U16& depth);
  void get_bestmove(position& p);
  double estimate_max_time(position& p, limits& lims);

  template<Nodetype type>
  Score search(position& p, int16 alpha, int16 beta, U16 depth, node * stack);

  template<Nodetype type>
  Score qsearch(position& p, int16 alpha, int16 beta, U16 depth, node * stack);
}


#include "search.hpp"

#endif
