#ifndef MOVE_H
#define MOVE_H

#include <vector>

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
  legal,
  pseudo_legal,
  no_type
};

class Move {
  int _value; 
  U8 _from;
  U8 _to;
  Movetype _type;
  Piece _piece;  
  
 public:
  Move() {
    _value = 0;
    _from = 0;
    _to = 0;
    _type = no_type;
    _piece = no_piece; 
  }
  Move(const U16& mv, int val = 0) {
    _value = val;
    _from = U8(mv & 0x3f);
    _to = U8((mv & 0xfc0) >> 6);
    _type = Movetype((mv & 0xf000) >> 12);    
  }
  Move(U8 f, U8 t, Movetype type) {
    _value = 0;
    _from = f;
    _to = t;
    _type = type;
  }
  Move(const Move& o);
  Move(const Move&& o);
  Move& operator=(const Move& o);
  Move& operator=(const Move&& o);
  ~Move() {}

  bool operator()(const Move& o) const; // { return o.m == m; }
  bool operator==(const Move& o) const;
  bool operator<(const Move& o) const { return _value < o.value(); }
  inline void set(const Piece& p,
		  const U8& f, const U8& t, const Movetype& type) {
    _piece = p; _value = 0; _from = f; _to = t; _type = type;
  }
  inline int value() const { return _value; }
  inline U8 from() const { return _from; }
  inline U8 to() const { return _to; }
  inline Movetype type() const { return _type; } 
  inline std::string to_string() { return SanSquares[_from] + SanSquares[_to]; }
};


class Movegen {
  int it, last;
  Move list[218]; // max moves in any chess position
  Color us, them;
  U64 rank2, rank7;
  U64 empty, pawns, pawns2, pawns7;
  std::array<Square, 11> knights, bishops, rooks, queens, kings;
  U64 enemies, all_pieces;
  Square eps;
  
  // utilities
  inline void initialize(const position& p);
  inline void pawn_pushes(U64& single, U64& dbl);
  inline void pawn_caps(U64& left, U64& right, U64& ep_left, U64& ep_right);
  inline void knight_quiets(std::vector<U64>& mvs);
  inline void knight_caps(std::vector<U64>& mvs);
  inline void bishop_mvs(std::vector<U64>& quiets, std::vector<U64>& caps);
  inline void rook_mvs(std::vector<U64>& quiets, std::vector<U64>& caps);
  inline void queen_mvs(std::vector<U64>& quiets, std::vector<U64>& caps);
  inline void king_mvs(std::vector<U64>& quiets, std::vector<U64>& caps);
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

  template<Movetype mt, Piece p>
  inline void generate();
  
  template<Piece p>
  inline void generate();
  
  template<Movetype mt>
  inline void generate();
  
  void print();
};

#include "move.hpp"

#endif
