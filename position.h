#pragma once

#ifndef POSITION_H
#define POSITION_H

#include <thread>
#include <algorithm>
#include <vector>
#include <iostream>
#include <array>
#include <string>
#include <sstream>
#include <cstring>

#include "types.h"
#include "utils.h"
#include "bitboards.h"
#include "magics.h"

struct Move;

struct info {
  U64 checkers;
  U64 pinned;
  U64 pkey;
  U64 dkey;
  U64 mkey;
  U64 pawnkey;
  U16 hmvs;
  U16 cmask;
  U8 move50;
  Color stm;
  Square eps;
  Square ks[2];
  Piece captured;    
  bool incheck;
};

struct piece_data {
  
  std::array<U64, 2> bycolor;
  std::array<Square, 2> king_sq;
  std::array<Color, squares> color_on;
  std::array<Piece, squares> piece_on;
  std::array<std::array<int, pieces>, 2> number_of;
  std::array<std::array<U64, squares>, colors> bitmap;
  std::array<std::array<std::array<int, squares>, pieces>, 2> piece_idx;
  std::array<std::array<std::array<Square, 11>, pieces>, 2> square_of;
  
  
  // utility methods for moving pieces
  void clear();

  void set(const Color& c, const Piece& p, const Square& s);
  
  inline void do_quiet(const Color& c, const Piece& p, const Square& f, const Square& t);

  template<Color c>
  inline void do_castle(const bool& kingside);
  
  template<Color c>
  inline void undo_castle(const bool& kingside);

  inline void do_cap(const Color& c, const Piece& p, const Square& f, const Square& t);
  
  inline void do_ep(const Color& c, const Square& f, const Square& t);

  inline void do_promotion(const Color& c, const Piece& p,
			   const Square& f, const Square& t);
	     
  inline void do_promotion_cap(const Color& c,
			const Piece& p, const Square& f, const Square& t);

  inline void do_castle_ks(const Color& c, const Square& f, const Square& t);
  
  inline void do_castle_qs(const Color& c, const Square& f, const Square& t);
  
  inline void remove_piece(const Color& c, const Piece& p, const Square& s);
  
  inline void add_piece(const Color& c, const Piece& p, const Square& s);
};

class position {  
  std::thread owner;  
  info history[512];
  info ifo;
  piece_data pcs;
  U64 hidx;
  
 public:
  position() {}
  position(std::istringstream& s);
  position(const std::string& fen);
  position(const position& p, const std::thread& t);
  position(const position& p);
  position(const position&& p);
  position& operator=(const position&);
  position& operator=(const position&&);
  ~position() { } 

  // setup/clear a position
  void setup(std::istringstream& fen);
  void clear();
  void set_piece(const char& p, const Square& s);
  void print();  
  void do_move(const Move& m);
  void undo_move(const Move& m);

  // utilities
  bool is_attacked(const Square& s, const Color& us, const Color& them, U64 m = 0ULL);
  U64 attackers_of(const Square& s, const Color& c);
  U64 checkers() const { return ifo.checkers; }
  bool in_check();
  bool is_legal(const Move& m);
  U64 pinned();
  inline bool can_castle_ks() const {
    return (ifo.cmask & (ifo.stm == white ? wks : bks)) == (ifo.stm == white ? wks : bks);
  }
  
  inline bool can_castle_qs() const {
    return (ifo.cmask & (ifo.stm == white ? wqs : bqs)) == (ifo.stm == white ? wqs : bqs);
  }
  
  // position info access wrappers
  inline Square eps() const { return ifo.eps; }
  inline Color to_move() const { return ifo.stm; }
  
  // piece access wrappers
  inline U64 all_pieces() const { return pcs.bycolor[white] | pcs.bycolor[black]; }

  inline Piece piece_on(const Square& s) const { return pcs.piece_on[s]; }
  
  inline Square king_square(const Color& c) const { return ifo.ks[c]; }

  inline Square king_square() const { return ifo.ks[ifo.stm]; }

  inline Color color_on(const Square& s) { return pcs.color_on[s]; }
  
  template<Color c, Piece p>
  inline U64 get_pieces() const { return pcs.bitmap[c][p]; }

  template<Color c>
  inline U64 get_pieces() const { return pcs.bycolor[c]; }

  template<Color c, Piece p>
  inline Square * squares_of() const {
    return const_cast<Square*>(pcs.square_of[c][p].data()+1);
  }
  
};


inline void piece_data::clear() {
  std::fill(bycolor.begin(), bycolor.end(), 0);
  std::fill(king_sq.begin(), king_sq.end(), Square::no_square);
  std::fill(color_on.begin(), color_on.end(), Color::no_color);
  std::fill(piece_on.begin(), piece_on.end(), Piece::no_piece);

  for (auto& v: number_of) std::fill(v.begin(), v.end(), 0);
  for (auto& v : bitmap) std::fill(v.begin(), v.end(), 0ULL);
  for (auto& v: piece_idx) { for (auto& w : v) { std::fill(w.begin(), w.end(), 0); } }
  for (auto& v: square_of) { for (auto& w : v) { std::fill(w.begin(), w.end(), Square::no_square); } }
}

inline void piece_data::do_quiet(const Color& c, const Piece& p,
				 const Square& f, const Square& t) {

  // bitmaps
  U64 fto = bitboards::squares[f] | bitboards::squares[t];

  int idx = piece_idx[c][p][f];
  piece_idx[c][p][f] = 0;
  piece_idx[c][p][t] = idx;
  
  bycolor[c] ^= fto;
  bitmap[c][p] ^= fto;
  
  square_of[c][p][idx] = t;
  color_on[f] = no_color;
  color_on[t] = c;

  piece_on[t] = p;
  piece_on[f] = no_piece;
  
  // zobrist update
}

inline void piece_data::do_cap(const Color& c, const Piece& p,
			const Square& f, const Square& t) {
  Color them = Color(c ^ 1);
  Piece cap = piece_on[t];
  remove_piece(them, cap, t);
  do_quiet(c, p, f, t);
}

inline void piece_data::do_promotion(const Color& c, const Piece& p,
			      const Square& f, const Square& t) {
  remove_piece(c, Piece::pawn, f);
  add_piece(c, p, t);
}
  
inline void piece_data::do_ep(const Color& c, const Square& f, const Square& t) {
  Color them = Color(c ^ 1);
  Square cs = Square(them == white ? t + 8 : t - 8);
  remove_piece(them, Piece::pawn, cs);
  do_quiet(c, Piece::pawn, f, t);
}

inline void piece_data::do_promotion_cap(const Color& c, const Piece& p,
				  const Square& f, const Square& t) {
  Color them = Color(c ^ 1);
  Piece cap = piece_on[t];
  remove_piece(them, cap, t);
  remove_piece(c, Piece::pawn, f);
  add_piece(c, p, t);
}

inline void piece_data::do_castle_ks(const Color& c, const Square& f, const Square& t) {
  Square rf = (c == white ? H1 : H8);
  Square rt = (c == white ? F1 : F8);
  
  do_quiet(c, king, f, t);
  do_quiet(c, rook, rf, rt);
}

inline void piece_data::do_castle_qs(const Color& c, const Square& f, const Square& t) {
  Square rf = (c == white ? A1 : A8);
  Square rt = (c == white ? D1 : D8);
  
  do_quiet(c, king, f, t);
  do_quiet(c, rook, rf, rt);    
}

inline void piece_data::remove_piece(const Color& c, const Piece& p, const Square& s) {
  U64 sq = bitboards::squares[s];
  bycolor[c] ^= sq;
  bitmap[c][p] ^= sq;

  // carefully remove this piece so when we add it back in undo, we
  // do not overwrite an existing piece index
  int tmp_idx = piece_idx[c][p][s];
  int max_idx = number_of[c][p];
  Square tmp_sq = square_of[c][p][max_idx];
  square_of[c][p][tmp_idx] = square_of[c][p][max_idx];
  square_of[c][p][max_idx] = no_square;
  piece_idx[c][p][tmp_sq] = tmp_idx;
  number_of[c][p] -= 1;
  piece_idx[c][p][s] = 0;
  color_on[s] = no_color;
  piece_on[s] = no_piece;
  // zobrist update 
}

inline void piece_data::add_piece(const Color& c, const Piece& p, const Square& s) {  
  U64 sq = bitboards::squares[s];
  bycolor[c] |= sq;
  bitmap[c][p] |= sq;

  // bug here - we are overwriting the "other" piece -- see previous code for how this was handled..
  // e.g. if we have 2-rooks, one is captured, then we undo the capture, number_of = 2 (again)
  
  number_of[c][p] += 1;
  square_of[c][p][number_of[c][p]] = s;
  piece_on[s] = p;
  piece_idx[c][p][s] = number_of[c][p];
  color_on[s] = c;
  //zobrist update
}

inline void piece_data::set(const Color& c, const Piece& p, const Square& s) {
  bitmap[c][p] |= bitboards::squares[s];
  bycolor[c] |= bitboards::squares[s];
  color_on[s] = c;
  number_of[c][p] += 1;
  piece_idx[c][p][s] = number_of[c][p];
  square_of[c][p][number_of[c][p]] = s;
  piece_on[s] = p;
  if (p == Piece::king) king_sq[c] = s;
}

#endif
