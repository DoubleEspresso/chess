#ifndef POSITION_H
#define POSITION_H

#include <thread>
#include <algorithm>
#include <vector>
#include <queue>

#include "types.h"
#include "utils.h"
#include "bitboards.h"

  struct checkinfo {
    U64 checkers;
    U64 disc_checks;
    U64 contact;
    U64 forks;
    U64 more_than_one;
    U64 pinned;
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
  };

struct piece_data {  
  std::vector<U64> bycolor{std::vector<U64>(2)};
  std::vector<std::vector<U64>> bitmap{std::vector<std::vector<U64>>(2, std::vector<U64>(squares))};
  std::vector<Square> king_sq { std::vector<Square>(2) };
  std::vector<Color> color_on { std::vector<Color>(squares) };
  std::vector<std::vector<std::vector<Square>>> square_of{std::vector< std::vector< std::vector<Square>>>(2, std::vector<std::vector<Square>>(Piece::pieces, std::vector<Square>(11)))}; 
  std::vector<Piece> piece_on { std::vector<Piece>(squares) }; 
  std::vector<std::vector<int>> number_of {std::vector<std::vector<int>>(2, std::vector<int>(Piece::pieces))};
  std::vector<std::vector<std::vector<int>>> piece_idx{std::vector<std::vector<std::vector<int>>>(2, std::vector<std::vector<int>>(Piece::pieces, std::vector<int>(squares)))};
  
  
  // utility methods for moving pieces
  void clear();
  
  template<Color c, Piece p>
  void do_quiet(const Square& f, const Square& t);

  template<Color c>
  void do_castle(const bool& kingside);

  template<Color c>
  void undo_castle(const bool& kingside);

  template<Color c, Piece p>
  void do_cap(const Square& f, const Square& t);
  
  template<Color c>
  void do_ep(const Square& f, const Square& t);

  template<Color c, Piece p>
  void do_promotion(const Square& f, const Square& t);

  template<Color c, Piece p>
  void do_promotion_cap(const Square& f, const Square& t);

  void remove_piece(const Color& c, const Piece& p, const Square& s);
  void add_piece(const Color& c, const Piece& p, const Square& s);
};

class position {  
  std::thread owner;  
  std::unique_ptr<checkinfo> ci;
  std::queue<info> history;
  info ifo;
  piece_data pcs;
  
 public:
  position();
  position(std::istringstream& s);
  position(const std::string& fen);
  position(const position& p, const std::thread& t);
  position(const position& p);
  position(const position&& p);
  position& operator=(const position&);
  position& operator=(const position&&);
  ~position();

  // setup/clear a position
  void setup(std::istringstream& fen);
  void clear();
  void set_piece(const char& p, const Square& s);
  void print();
  //void do_move(info &ifo, const U16& m);
  //void undo_move(const U16& m);
};


inline void piece_data::clear() {  
  std::fill(bycolor.begin(), bycolor.end(), 0);
  for (auto& v : bitmap) std::fill(v.begin(), v.end(), 0ULL);
  std::fill(king_sq.begin(), king_sq.end(), Square::no_square);
  std::fill(color_on.begin(), color_on.end(), Color::no_color);
  for (auto& v: square_of) { for (auto& w : v) { std::fill(w.begin(), w.end(), Square::no_square); } }
  std::fill(piece_on.begin(), piece_on.end(), Piece::no_piece);
  for (auto& v: number_of) std::fill(v.begin(), v.end(), 0);
  for (auto& v: piece_idx) { for (auto& w : v) { std::fill(w.begin(), w.end(), 0); } }
}

template<Color c, Piece p>
  void piece_data::do_quiet(const Square& f, const Square& t) {
  // bitmaps
  U64 fto = bitboards::squares[f] | bitboards::squares[t];

  int idx = piece_idx[c][p][f];
  piece_idx[c][p][f] = 0;
  piece_idx[c][p][t] = idx;
  
  bycolor[c] ^= fto;
  bitmap[c][p] ^= fto;
  
  square_of[c][p][idx] = t;
  color_on[f] = no_color; color_on[t] = c;
  piece_on[t] = p; piece_on[f] = no_piece;
  
  // zobrist update
}

template<Color c, Piece p>
  void piece_data::do_cap(const Square& f, const Square& t) {
  Color them = c ^ 1;
  Piece cap = piece_on[t];
  remove_piece(them, cap, t);
  do_quiet<c, p>(f, t);
}

template<Color c, Piece p>
void piece_data::do_promotion(const Square& f, const Square& t) {
  remove_piece(c, Piece::pawn, f);
  add_piece(c, p, t);
}

template<Color c> 
void piece_data::do_ep(const Square& f, const Square& t) {
  Color them = c ^ 1;
  Square cs = (them == white ? t + 8 : t - 8);
  remove_piece(them, Piece::pawn, cs);
  do_quiet<c, Piece::pawn>(f, t);
}

template<Color c, Piece p>
  void piece_data::do_promotion_cap(const Square& f, const Square& t) {
  Color them = c ^ 1;
  Piece cap = piece_on[t];
  remove_piece(them, cap, t);
  remove_piece(c, Piece::pawn, f);
  add_piece(c, p, t);
}

inline void piece_data::remove_piece(const Color& c, const Piece& p, const Square& s) {
  U64 sq = bitboards::squares[s];
  bycolor[c] ^= sq;
  bitmap[c][p] ^= sq;
  
  int idx = piece_idx[c][p][s];
  piece_idx[c][p][s] = 0;
  square_of[c][p][idx] = no_square;
  number_of[c][p] -= 1;
  color_on[s] = no_color;
  piece_on[s] = no_piece;
  // zobrist update 
}

inline void piece_data::add_piece(const Color& c, const Piece& p, const Square& s) {  
  U64 sq = bitboards::squares[s];
  bycolor[c] |= sq;
  bitmap[c][p] |= sq;

  number_of[c][p] += 1;
  square_of[c][p][number_of[c][p]] = s;
  piece_on[s] = p;
  piece_idx[c][p][s] = number_of[c][p];
  color_on[s] = c;
  //zobrist update
}



#endif
