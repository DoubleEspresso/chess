#ifndef SEARCH_H
#define SEARCH_H

#include "types.h"
#include "threads.h"

enum Nodetype { root, pv, non_pv };

namespace Search {
  
  template<Nodetype type>
  Score search(position& p, int16 alpha, int16 beta, U16 depth);
  
}

extern Threadpool search_threads;

#include "search.hpp"

#endif
