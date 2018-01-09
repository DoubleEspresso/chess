#pragma once
#ifndef SBC_MOVE2_H
#define SBC_MOVE2_H

#include <stdio.h>

#include "definitions.h"
#include "types.h"
#include "globals.h"


enum MoveType2 {
  QUIETS,
  CHECKS,
  QUIET_CHECKS,
  QUIET_NOCHECKS,
  CAPTURES,
  //CAPTURE_CHECKS,
  CAPTURE_NOCHECKS,
  PROMOTIONS,
  CAPTURE_PROMOTIONS,
  CASTLES,
  DANGEROUS_QUIETS,
  DANGEROUS_CHECKS,
  ALL_MVS
};


struct Move2 {
  U16 m;
  int v;

  Move2() : m(MOVE_NONE), v(0) {}
  Move2(const Move2& o) : m(o.m), v(o.v) { }
  bool operator==(const Move2& o) { return m == o.m; }
};

template <MoveType2 T, Piece p, Color c>
class MoveGenerator2 {  
  int it, last;
  Move2 list[MAX_MOVES];

  void generate(Board& b);
  template<Direction> void serialize(U64& b);
  
 public:
  MoveGenerator2() : it(0), last(0) {}
  MoveGenerator2<T,p,c>(Board& b) : it(0), last(0) { generate(b); }

  void print();
  Move2& operator++() { return list[it++]; }
  Move2& operator()() { return list[it]; }
  Move2& operator()(const int i) { return list[i]; } // no bounds check

}; 

#include "move2.hpp"

#endif
