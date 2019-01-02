#include "position.h"

Threadpool search_threads(4);

template<Nodetype type>
int16 Search::go(position& p, int16 alpha, int16 beta, U16 depth) {

  p.print();
  std::cout << "..searching, alpha = " << alpha << std::endl;
  return 0;
}
