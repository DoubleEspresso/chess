#pragma once

#ifndef MOVE_H
#define MOVE_H

#include <vector>
#include <array>

#include "types.h"

class position;

enum Dir { N, S, NN, SS, NW, NE, SW, SE, none };

enum Movetype {
  promotion,
  capture_promotion=4,
  castle_ks=8,
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
  legal,
  pseudo_legal_quiet,
  pseudo_legal_capture,
  pseudo_legal_all,
  no_type
};

class Move {
  int _value; 
  Square _from;
  Square _to;
  Movetype _type;
  Piece _piece;
  Piece _promote;
  
  
 public:
  Move() {
    _value = 0;
    _from = A1;
    _to = A1;
    _type = no_type;
    _piece = no_piece;
    _promote = no_piece;
  }
  Move(const U16& mv, int val = 0) {
    _value = val;
    _from = Square(mv & 0x3f);
    _to = Square((mv & 0xfc0) >> 6);
    _type = Movetype((mv & 0xf000) >> 12);    
  }
  Move(Piece p, Square f, Square t, Movetype type) {
    _piece = p;
    _value = 0;
    _from = f;
    _to = t;
    _type = type;
    if (_type >= 0 && _type < capture_promotion) {
      _promote = (_type == 0 ? queen :
		  _type == 1 ? rook :
		  _type == 2 ? bishop : knight);      
    }
    else if (_type >= capture_promotion && _type < castle_ks) {
      _promote = (_type == 4 ? queen :
		  _type == 5 ? rook :
		  _type == 6 ? bishop : knight);
    }
    else _promote = no_piece;
  }  
  Move(const Move& o) = default;
  Move(Move&& o) = default;
  Move& operator=(Move&& o) = default;
  Move& operator=(const Move& o) = default;
  ~Move() {}
  
  bool operator()(const Move& o) const; // { return o.m == m; }
  bool operator==(const Move& o) const;
  bool operator<(const Move& o) const { return _value < o.value(); }
  
  inline int value() const { return _value; }
  inline Square from() const { return _from; }
  inline Square to() const { return _to; }
  inline Movetype type() const { return _type; }
  inline Piece piece() const { return _piece; }
  inline Piece promote() const { return _promote; }
  inline std::string to_string() {
    if (_type < castle_ks) { // promotions
      const std::string p = "qrbn";
      return SanSquares[_to] + p[(_type < capture_promotion ? _type : _type - 4)];
    }
    return SanSquares[_from] + SanSquares[_to];
      
  }
};


class Movegen {
  int it, last;
  Move list[218]; // max moves in any chess position
  Color us, them;
  U64 rank2, rank7;
  U64 empty, pawns, pawns2, pawns7;
  std::vector<U64> bishop_mvs, rook_mvs, queen_mvs;
  //std::array<Square, 10> knights, bishops, rooks, queens, kings;
  Square * knights;
  Square * bishops;
  Square * rooks;
  Square * queens;
  Square * kings;
  U64 enemies, all_pieces, check_target, evasion_target; // qtarget, ctarget;
  Square eps;
  bool can_castle_ks, can_castle_qs;
  
  // utilities
  inline void initialize(const position& p);
  inline void pawn_quiets(U64& single, U64& dbl);
  inline void pawn_caps(U64& left, U64& right, U64& ep_left, U64& ep_right);
  inline void quiet_promotions(U64& quiets);
  inline void capture_promotions(U64& right_caps, U64& left_caps);
  
  template<Movetype mt, Piece p>
  inline void encode(U64& b, const int& f);

  template<Movetype mt, Piece p>
  inline void encode_pawn_pushes(U64& b, const int& dir);
  
  template<Movetype mt, Piece p>
  inline void encode_promotions(U64& b, const int& f);
  
 public:
  Movegen() : it(0), last(0) {}
  Movegen(const position& pos) : it(0), last(0) { initialize(pos); }
  Movegen(const Movegen& o) = delete;
  Movegen(const Movegen&& o) = delete;
  
  Movegen& operator=(const Movegen& o) = delete;
  Movegen& operator=(const Movegen&& o) = delete;
  Move& operator[](const int& idx) { return list[idx]; }

  inline int size() { return last; }
  inline Move& move(int i) { return list[i]; }
  
  template<Movetype mt, Piece p>
  inline void generate();
  
  template<Piece p>
  inline void generate();
  
  template<Movetype mt>
  inline void generate();
  
  inline void print();
  inline void print_legal(position& p);
};

#include "move.hpp"

#endif
