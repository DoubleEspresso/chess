#ifndef POSITION_H
#define POSITION_H

#include <thread>
#include <algorithm>
#include <vector>

#include "types.h"
#include "utils.h"


struct pieces {
  std::vector<U64> bycolor(2);
  std::vector<std::vector<U64>> bitmap(2, std::vector<U64>(squares));
  std::vector<Square> king_sq(2);
  std::vector<Color> color_on(squares);
  std::vector<std::vector<std::vector<Square>>> square_of(2, std::vector<Square>(pieces, std::vector<Square>(11)));
  std::vector<Piece> piece_on(squares);
  std::vector<std::vector<Piece>> number_of(2, std::vector<Piece>(pieces));
  std::vector<std::vector<std::vector<int>>> piece_idx(2, std::vector<int>(pieces, std::vector<int>(squares)));
  
  // utility methods for moving pieces
  void clear();
  
  template<typename Color, typename Piece>
  void moveto_quiet(const Square& f, const Square& t);

  template<typename Color>
  void do_castle(const bool& kingside);

  template<typename Color>
  void undo_castle(const bool& kingside);
  
  template<typename Color, typename Piece>
  void moveto_cap(const Square& f, const Square& t);
  
  template<typename Color, typename Piece>
  void remove_piece(const Square& sq);
  
  template<typename Color, typename Piece>
  void add_piece(const Square& sq);
};

inline void Pieces::clear() {  
  std::fill(bycolor.begin(), bycolor.end(), 0);
  for (auto& v : bitmap) std::fill(v.begin(), v.end(), 0);
  std::fill(king_sq.begin(), king_sq.end(), 0);
  std::fill(color_on.begin(), color_on.end(), 0);
  for (auto& v: square_of) { for (auto& w : v) { for (auto &x : w) std::fill(x.begin(), x.end(), 0); } }
  std::fill(piece_on.begin(), piece_on.end(), 0);
  for (auto& v: number_of) { for (auto& w : v) std::fill(w.begin(), w.end(), 0); }
  for (auto& v: piece_idx) { for (auto& w : v) { for (auto &x : w) std::fill(x.begin(), x.end(), 0); } }
}

template<typename Color, typename Piece>
  void Pieces::moveto_quiet(const Square& f, const Square& t) {
  // bitmaps
  U64 fto = bitboards::squares[f] | bitboards::squares[t];
  bycolor[Color] ^= fto;
  bitmap[Color][Piece] ^= fto;

  square_of[Color][Piece][idx] = t;
  color_on[f] = no_color; color_on[t] = Color;
  piece_on[t] = Piece; piece_on[f] = no_piece;
  
  int idx = piece_idx[Color][Piece][f];
  piece_idx[Color][Piece][f] = 0;
  piece_idx[Color][Piece][t] = idx;

  // zobrist update
}


class position {

  struct checkinfo {
    U64 checkers;
    U64 disc_checks;
    U64 contact;
    U64 forks;
    U64 more_than_one;
    U64 blockers; // pieces blocking disc checks
  };

  struct info {
    Color stm;
    Square eps;
    bool incheck;
    Piece captured;
    U8 move50;
    U16 hmvs;
    U16 castle_mask;
    U64 key; // position
    U64 mKey; // material
    U64 pKey; // pawn
    U64 dKey; // data ?
    U64 pinned;
    std::unique_ptr<checkinfo> ci;
  };
  
  std::thread owner;  
  std::unique_ptr<position> previous;
  std::queue<info> history;
  pieces pcs;
  
 public:
  position();
  position(const std::istringstream& s);
  position(const position& p, const std::thread& t);
  position(const position& p);
  position(const position&& p);
  position& operator=(const position&);
  position& operator=(const position&&);
  ~position();

  std::thread worker() { return owner; }
  void set_worker(std::thread& t) { owner = t; }
  
};

#endif
