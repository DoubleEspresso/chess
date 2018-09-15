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

  inline int from() { return int(m & 0x3f); }
  inline int to() { return int((m & 0xfc0) >> 6); }
  inline int type() { return int((m & 0xf000) >> 12); }  
  inline std::string to_string() { return SanSquares[from()] + SanSquares[to()]; }
  
  U16 m;
  int v; 
};
//inline bool mvgreater(const move& x, const move& y) { return x.v > y.v; }

enum Dir { N, S, NN, SS, NW, NE, SW, SE, none };
enum Movetype {
  promotion,
  capture_promotion,
  castle_ks,
  castle_qs,
  quiet,
  capture,
  ep,
  evasion,
  check,
  castles,
  quiet_checks,
  quiet_nochecks,
  capture_nochecks,
  advanced_pawn_pushes,
  dangerous_checks,
  all_legal,
  all_pseudo_legal
};

template<Movetype mt, Piece p, Color c>
class movegen {
  int it, last;
  move list[218]; // max moves in any chess position
  
  inline void generate(const position& pos);
  inline void encode(U64& b, const int& f);
  inline void encode_pawn_pushes(U64& b, const int& dir);
  inline void encode_promotions(U64& b, const int& f);
  
 public:
  movegen() : it(0), last(0) {}
  movegen(const position& pos) : it(0), last(0) { generate(pos); }
  movegen(const movegen& o) = delete;
  movegen(const movegen&& o) = delete;
  movegen& operator=(const movegen& o) = delete;
  movegen& operator=(const movegen&& o) = delete;

  void print();
};

#include "move.hpp"

#endif
