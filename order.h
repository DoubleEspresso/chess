#ifndef ORDER_H
#define ORDER_H


#include <array>
#include <vector>
#include <memory>
#include <algorithm>
#include <iostream>

#include "types.h"

struct Move;
class Movegen;
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


struct scored_move {
  scored_move(Move& mv, Score& sc) : m(mv), s(sc) { }
  scored_move(Move&& mv, Score&& sc) : m(mv), s(sc) { }
  Move& m;
  Score s;
  bool operator>(const scored_move& o) { return s > o.s; }
  bool operator<(const scored_move& o) { return s < o.s; }
  scored_move& operator=(const scored_move& o);
};


enum search_type { main, qsearch, nullsearch, no_search };


class move_order {

  // for reference:
  // enum OrderPhase { hash_move, mate_killer1, mate_killer2, good_captures,
  //		  killer1, killer2, bad_captures, quiets, end };

  OrderPhase phase;
  Move * hashmove;
  move_history * stats;
  Movegen * moves;
  Color to_move;
  bool incheck;
  std::vector<scored_move> list;

 public:
  move_order() { }
  move_order(move_history& mh, const search_type& t);
  move_order(position& p, Move& hashmove);
  move_order(const move_order& mo) = delete;
  move_order(const move_order&& mo) = delete;  
  move_order& operator=(const move_order& o) = delete;
  move_order& operator=(const move_order&& o) = delete;  
  ~move_order();

  template<search_type st>
  bool next_move(Move& m);
  
  void sort();
  bool skip(const Move& m);
  
};


#endif
