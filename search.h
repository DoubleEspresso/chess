#ifndef SEARCH_H
#define SEARCH_H

#include "types.h"
#include "threads.h"

enum Nodetype { root, pv, non_pv };

namespace Search {
  
  std::atomic_bool is_searching;
  
  void search_timer();
  void start(position& p, U16 depth);
  void iterative_deepening(position& p, U16 depth);  
  
  template<Nodetype type>
  Score search(position& p, int16 alpha, int16 beta, U16 depth);
}

#include "search.hpp"

#endif
