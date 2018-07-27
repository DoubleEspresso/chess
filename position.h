#ifndef POSITION_H
#define POSITION_H

#include <thread>
#include <algorithm>
#include <vector>

#include "types.h"
#include "utils.h"



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
  void do_quiet(const Square& f, const Square& t);

  template<typename Color>
  void do_castle(const bool& kingside);

  template<typename Color>
  void undo_castle(const bool& kingside);
  
  template<typename Color, typename Piece>
  void do_cap(const Square& f, const Square& t);
  
  template<typename Color>
  void do_ep(const Square& f, const Square& t);

  template<typename Color, typename Piece>
  void do_promotion(const Square& f, const Square& t);

  template<typename Color, typename Piece>
  void do_promotion_cap(const Square& f, const Square& t);

  void remove_piece(const Color& c, const Piece& p, const Square& s);
  void add_piece(const Color& c, const Piece& p, const Square& s);
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
  void Pieces::do_quiet(const Square& f, const Square& t) {
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

template<typename Color, typename Piece>
  void Pieces::do_cap(const Square& f, const Square& t) {
  Color them = Color ^ 1;
  Piece cap = piece_on[t];
  remove_piece(them, cap, t);
  moveto_quiet<Color, Piece>(f, t);
}

template<typename Color, typename Piece>
void Pieces::do_promotion(const Square& f, const Square& t) {
  remove_piece(Color, Piece::pawn, f);
  add_piece(Color, Piece, t);
}

template<typename Color>
void Pieces::do_ep(Square& f, Square& t) {
  Color them = Color ^ 1;
  Square cs = (them == white ? t + 8 : t - 8);
  remove_piece(them, Piece::pawn, cs);
  do_quiet<Color, Piece::pawn>(f, t);
}

template<typename Color, typename Piece>
  void Pieces::do_promotion_cap(const Square& f, const Square& t) {
  Color them = Color ^ 1;
  Piece cap = piece_on[t];
  remove_piece(them, cap, t);
  remove_piece(Color, Piece::pawn, f);
  add_piece(Color, Piece, t);
}

void Pieces::remove_piece(const Color& c, const Piece& p, const Square& s) {
  U64 sq = bitboards::squares[s];
  bycolor[c] ^= sq;
  bitmap[c][p] ^= sq;

  int idx = piece_index[c][p][s];
  piece_index[c][p][s] = 0;
  square_of[c][p][idx] = no_square;
  number_of[c][p]--;
  color_on[s] = no_color;
  piece_on[s] = no_piece;
  // zobrist update 
}

void Pieces::add_piece(const Color& c, const Piece& p, const Square& s) {  
  U64 sq = bitboards::squares[s];
  bycolor[c] |= sq;
  bitmap[c][p] |= sq;

  number_of[c][p]++;
  square_of[c][p][number_of[c][p]] = s;
  piece_on[s] = p;
  piece_idx[c][p][s] = number_of[c][p];
  color_on[s] = c;
  //zobrist update
}



#endif
