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
#pragma once

#ifndef ORDER_H
#define ORDER_H


#include <array>
#include <vector>
#include <memory>
#include <algorithm>
#include <iostream>

#include "types.h"

class position;
class Movegen;


struct move_history {
  std::array<std::array<std::array<int, squares>, squares>, colors> history;
  std::array<std::array<Move, squares>, squares> counters;

  float counter_move_bonus = 1.0f;
  float threat_evasion_bonus = 1.0f;

  move_history() { clear(); }
    
  move_history& operator=(const move_history& mh);
  
  
  void update(const position& p,
	      const Move& m,
	      const Move& previous,
	      const int16& depth,
	      const Score& eval,
	      const std::vector<Move>& quiets,
	      Move * killers);
  
  void clear();
  
  template<Color c> int score(const Move& m, const Move& previous, const Move& followup, const Move& threat);
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


enum search_type { main0, qsearch, nullsearch, no_search };


class move_order {

  // for reference:
  // enum OrderPhase { hash_move, mate_killer1, mate_killer2, good_captures,
  //		  killer1, killer2, bad_captures, quiets, end };

  OrderPhase phase;
  Move * hashmove;
  move_history * stats;
  Movegen * moves;
  Move * killers;
  Color to_move;
  bool incheck;
  std::vector<scored_move> list;

 public:
  move_order() { }
  move_order(position& p, Move& hashmove, Move * kill);
  move_order(const move_order& mo) = delete;
  move_order(const move_order&& mo) = delete;  
  move_order& operator=(const move_order& o) = delete;
  move_order& operator=(const move_order&& o) = delete;  
  ~move_order();

  template<search_type st>
  bool next_move(position& pos, Move& m, const Move& previous, const Move& followup, const Move& threat);
  
  void sort();
  bool skip(const Move& m);
  
};


#endif
