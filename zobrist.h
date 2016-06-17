#pragma once
#ifndef HEDWIG_ZOBRIST_H
#define HEDWIG_ZOBRIST_H

#include <stdio.h>

#include "definitions.h"
#include "types.h"

namespace Zobrist
{
  bool init();
  U64 piece_rands(int square, int color, int piece);
  U64 castle_rands(int color, U16 castle_rights);
  U64 ep_rands(int column);
  U64 stm_rands(int c);
  U64 mv50_rands(int mv);

  extern U64 zobrist_rands[SQUARES][2][PIECES];
  extern U64 zobrist_cr[2][16];
  extern U64 zobrist_col[8];
  extern U64 zobrist_stm[2];
  extern U64 zobrist_move50[50];
};

#endif
