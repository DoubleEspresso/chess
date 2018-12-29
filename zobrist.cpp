#include "zobrist.h"

namespace zobrist {
  U64 piece_rands[Square::squares][2][pieces];
  U64 castle_rands[2][16];
  U64 ep_rands[8];
  U64 stm_rands[2];
  U64 move50_rands[512];
  U64 hmv_rands[512];
}



bool zobrist::load() {
  
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


  for (int m = 0; m < 512; ++m) {
    move50_rands[m] = gen.next();
    hmv_rands[m] = gen.next();
  }
  
  return true;
}


U64 zobrist::piece(const Square& s, const Color& c, const Piece& p) {
  return piece_rands[s][c][p];
}

U64 zobrist::castle(const Color& c, const U16& bit) {
  return castle_rands[c][bit];
}

U64 zobrist::ep(const U8& column) {
  return ep_rands[column];
}

U64 zobrist::stm(const Color& c) {
  return stm_rands[c];
}

U64 zobrist::mv50(const U16& count) {
  return move50_rands[count];
}

U64 zobrist::hmvs(const U16& count) {
  return hmv_rands[count];
}
