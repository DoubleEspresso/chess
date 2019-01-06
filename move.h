#pragma once

#ifndef MOVE_H
#define MOVE_H

#include <vector>
#include <array>

#include "types.h"
#include "utils.h"

class position;

enum Dir { N, S, NN, SS, NW, NE, SW, SE, none };

enum Movetype {
  promotion_q,
  promotion_r,
  promotion_b,
  promotion_n,
  capture_promotion_q,
  capture_promotion_r,
  capture_promotion_b,
  capture_promotion_n,
  castle_ks,
  castle_qs,
  quiet,
  capture,
  ep,
  castles,
  pseudo_legal,
  promotion,
  capture_promotion,
  no_type
};

struct Move {
  U8 f;
  U8 t;
  U8 type;
  inline void set(const U8& frm, const U8& to, const Movetype& mt) {
    f = frm; t = to; type = mt;
  }
};

class Movegen {
  int it, last;
  Move list[218]; // max moves in any chess position
  Color us, them;
  U64 rank2, rank7;
  U64 empty, pawns, pawns2, pawns7;
  std::vector<U64> bishop_mvs, rook_mvs, queen_mvs;  
  Square * knights;
  Square * bishops;
  Square * rooks;
  Square * queens;
  Square * kings;
  U64 enemies, all_pieces, qtarget, ctarget, check_target, evasion_target;
  Square eps;
  bool can_castle_ks, can_castle_qs;
  
  // utilities  
  inline void initialize(const position& p);
  inline void pawn_caps(U64& left, U64& right, U64& ep_left, U64& ep_right);
  
  template<Movetype mt>
  inline void encode(U64& b, const int& f);

  template<Movetype mt>
  inline void encode_pawn_pushes(U64& b, const int& dir);

  inline void pawn_quiets(U64& single, U64& dbl);
  inline void encode_promotions(U64& b, const int& dir);
  inline void capture_promotions(U64& right_caps, U64& left_caps);
  inline void quiet_promotions(U64& quiets);
  inline void encode_capture_promotions(U64& b, const int& f);
  
 public:
  Movegen() : it(0), last(0) {}
  Movegen(const position& pos) : it(0), last(0) { initialize(pos); }
  Movegen(const Movegen& o) = delete;
  Movegen(const Movegen&& o) = delete;
  
  Movegen& operator=(const Movegen& o) = delete;
  Movegen& operator=(const Movegen&& o) = delete;
  Move& operator[](const int& idx) { return list[idx]; }
  
  template<Movetype mt, Piece p>
  inline void generate();

  // utilities
  inline int size() { return last; }    
  inline void print();
  inline void print_legal(position& p);
};

#include "move.hpp"

#endif
