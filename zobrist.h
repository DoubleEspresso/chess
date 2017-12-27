#pragma once
#ifndef SBC_ZOBRIST_H
#define SBC_ZOBRIST_H

#include <stdio.h>

#include "definitions.h"
#include "types.h"

namespace Zobrist {
  bool init();
  U64 piece_rands(const U8& square, const U8& color, const U8& piece);
  U64 castle_rands(const U8& color, const U16& castle_rights);
  U64 ep_rands(const U8&  column);
  U64 stm_rands(const U8&  c);
  U64 mv50_rands(const U8&  mv);

  extern U64 zobrist_rands[SQUARES][2][PIECES];
  extern U64 zobrist_cr[2][16];
  extern U64 zobrist_col[8];
  extern U64 zobrist_stm[2];
  extern U64 zobrist_move50[50];
}

#endif
