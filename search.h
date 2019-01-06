#ifndef SEARCH_H
#define SEARCH_H

#include <algorithm>

#include "types.h"
#include "threads.h"



namespace Search {
  
  std::atomic_bool is_searching;  
  std::mutex mtx;
  
  void search_timer();
  void start(position& p, U16 depth);
  void iterative_deepening(position& p, U16 depth, unsigned tid);
  void readout_pv(position& p, const Score& eval, const U16& depth);
  
  template<Nodetype type>
  Score search(position& p, int16 alpha, int16 beta, U16 depth, unsigned tid);
}


#include "search.hpp"

#endif
