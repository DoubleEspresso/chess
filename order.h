#ifndef ORDER_H
#define ORDER_H


#include <array>
#include <vector>
#include <memory>
#include <algorithm>
#include <iostream>

#include "types.h"

struct Move;
class position;

struct move_history {
  std::array<std::array<std::array<int, squares>, squares>, colors> history;
  
  move_history() { clear(); }
    
  move_history& operator=(const move_history& mh);
  
  
  void update(const position& p,
	      const Move& m,
	      const Move& previous,
	      const int16& depth,
	      const std::vector<Move>& quiets);  
  void clear();
  
  template<Color c>
  int score(const Move& m);
};


struct move_list {
  const Move& m;
  Score s;
  bool operator>(const move_list& o) { return s > o.s; }
  bool operator<(const move_list& o) { return s < o.s; }
};


enum search_type { main, qsearch, nullsearch, no_search };


class move_order {

  enum Phase { hash_move, mate_killer1, mate_killer2, good_captures,
	       killer1, killer2, bad_captures, quiets, end };
  //Phase phase;
  //search_type type;
  move_history * stats;
  std::vector<move_list> list;


 move_order(move_history& mh, const search_type& t)
   //: phase(hash_move), type(t), stats(0) {
   {
     stats = &mh;
   }  
 move_order(const move_order& mo) = delete;
 move_order(const move_order&& mo) = delete;  
 move_order& operator=(const move_order& o) = delete;
 move_order& operator=(const move_order&& o) = delete;  
 ~move_order() { }
 
  
};


#endif
