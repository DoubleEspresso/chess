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
  int history[colors][squares][squares];
  
  void update(const Move& m,
	      const Move& previous,
	      const int16& depth,
	      Move * quiets);
  
  inline void clear();
  
  template<Color c>
  int score(const Move& m);
};




#endif
