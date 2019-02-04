#ifndef ORDER_H
#define ORDER_H


#include <array>
#include <vector>
#include <memory>
#include <algorithm>
#include <iostream>

#include "types.h"

struct Move;


struct move_history {
  std::array<std::array<std::array<int, squares>, squares>, colors> history;
  
  move_history() { clear(); }
    
  move_history& operator=(const move_history& mh);
  
  
  void update(const Move& m,
	      const Move& previous,
	      const int16& depth,
	      Move * quiets);  
  void clear();
  
  template<Color c>
  int score(const Move& m);
};



#endif
