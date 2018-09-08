#ifndef MOVE_H
#define MOVE_H


#include "types.h"
#include "position.h"

//---------------------------------------------
// Move Encoding:
// Store move information in a 16-bit word, 
// using the standard 'from'-'to' representation.
// Broken down as follows,
//
// BITS                                DESCRIPTION
// 0-5                                 from sq (0-63)
// 6-11                                to sq   (0-63)
// 12-16                               move data
//
// last 4 bits
// 0000                                quite move 
// 0001                                capture bit
// 1000                                promotion bit
// 0010                                check bit
// 0100                                castle bit
//
// Quiet promotions
// 1000                                queen
// 1010                                rook
// 1100                                bishop
// 1110                                knight
//
// Capture promotions
// 1001                                queen
// 1011                                rook
// 1101                                bishop
// 1111                                knight
//
// Castle bits
// 0100                                castle k.s.
// 0110                                castle q.s.
//
// check info
// 0010                                check from minor (pawn/bishop/knight)
// 0011                                check from major (queen/rook)
//-------------------------------------------------

class move {
  U16 m;
  int v; 
 public:
  move() {}
  move(const U16& mv, int val = 0) : m(mv), v(val) {}
  move(const move& o);
  move(const move&& o);
  move& operator=(const move& o);
  move& operator=(const move&& o);
  ~move() {}

  bool operator()(const move& o) const { return o.m == m; }
  bool operator==(const move& o) { return o.m == m; }
};
//inline bool mvgreater(const move& x, const move& y) { return x.v > y.v; }


enum Movetype {
  promotions = 4,
  capture_promotions = 8,
  castle_ks,
  castle_qs,
  quiets,
  captures,
  evasions,
  checks,
  castles,
  quiet_checks,
  quiet_nochecks,
  capture_nochecks,
  advanced_pawn_pushes,
  dangerous_checks,
  all_legal,
  all_pseudo_legal
};

template<Movetype T, Color c>
class movegen {
  int it, last;
  move list[218]; // max moves in any chess position

  inline void generate(const position& p);

 public:
  movegen() : it(0), last(0) {}
  movegen(const position& p) : it(0), last(0) { generate(p); }
  movegen(const movegen& o) = delete;
  movegen(const movegen&& o) = delete;
  movegen& operator=(const movegen& o) = delete;
  movegen& operator=(const movegen&& o) = delete;
};

#endif
