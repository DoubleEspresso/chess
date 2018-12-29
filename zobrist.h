#pragma once

#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "types.h"
#include "utils.h"

namespace zobrist {
  inline bool load();
  inline U64 piece(const Square& s, const Color& c, const Piece& p);
  inline U64 castle(const Color& c, const U16& bit);
  inline U64 ep(const U8& column);
  inline U64 stm(const Color& c);
  inline U64 mv50(const U8& mv);

  namespace {
    U64 piece_rands[Square::squares][2][pieces];
    U64 castle_rands[2][16];
    U64 ep_rands[8];
    U64 stm_rands[2];
    U64 move50_rands[50];
  }
}


inline bool zobrist::load() {
  
  util::rand<U64> gen;

  // pieces
  for (Square sq = A1; sq <= H8; ++sq) {
    for (Color c = white; c <= black; ++c) {
      for (Piece p = pawn; p <= king; ++p) {
	piece_rands[sq][c][p] = gen.next();
      }
    }
  }


  // castle rights
  for (Color c = white; c <= black; ++c) {
    for (int bit = 0; bit < 16; ++bit) {
      castle_rands[c][bit] = gen.next();
    }
  }

  
  // ep
  for (int col = 0; col < 8; ++col) {
    ep_rands[col] = gen.next();
  }

  // stm
  stm_rands[white] = gen.next();
  stm_rands[black] = gen.next();


  for (int m = 0; m < 50; ++m) {
    move50_rands[m] = gen.next();
  }
  
  return true;
}


inline U64 zobrist::piece(const Square& s, const Color& c, const Piece& p) {
  return piece_rands[s][c][p];
}

inline U64 zobrist::castle(const Color& c, const U16& bit) {
  return castle_rands[c][bit];
}

inline U64 zobrist::ep(const U8& column) {
  return ep_rands[column];
}

inline U64 zobrist::stm(const Color& c) {
  return stm_rands[c];
}

inline U64 zobrist::mv50(const U8& mv) {
  return move50_rands[mv];
}

#endif
