#pragma once
#ifndef SBC_MOVE2_H
#define SBC_MOVE2_H

#include <stdio.h>

#include "definitions.h"
#include "types.h"
#include "globals.h"


enum MoveType2 {
  QUIET_PAWN,
  ADVANCED_PAWN,
  CAPTURE_PAWN,
  QUIET_PROMOTION,
  CAPTURE_PROMOTION,
  PAWN_CHECKS,
  QUIET_PIECE_NO_CHECK,
  QUITE_PIECE_CHECK,
  QUIET_PIECE,
  CAPTURE_PIECE_NO_CHECK,
  CAPTURE_PIECE_CHECK,
  CAPTURE_PIECE,
  CASTLES,
  PIECE_MVS,
  PAWN_MVS,
  DANGEROUS_MVS,
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

template <MoveType2 T>
class MoveGenerator2 {  
  int it, last;
  Move2 list[MAX_MOVES];

  void generate(const Board& b);

 public:
  MoveGenerator2() : it(0), last(0) {}
  MoveGenerator2(const Board& b) : it(0), last(0) { generate(b); }   

  void print();
  Move2& operator++() { return list[it++]; }
  Move2& operator()() { return list[it]; }
  Move2& operator()(const int i) { return list[i]; } // no bounds check

}; 

#include "move2.hpp"

#endif
