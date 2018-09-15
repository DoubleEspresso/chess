# pragma once

#ifndef BITBOARDS_H
#define BITBOARDS_H

#include "types.h"
#include "utils.h"

namespace bitboards {

  extern U64 row[8];
  extern U64 col[8];
  extern U64 color[2]; // white/black squares
  extern U64 pawnmask[2]; // 2nd - 6th rank mask for pawns (to exclude promotion candidates)
  extern U64 pawnmaskleft[2]; // 2nd - 6th rank mask for pawn captures
  extern U64 pawnmaskright[2]; // 2nd - 6th rank mask for pawn captures
  extern U64 bmask[64]; // bishop mask (outer board edges are trimmed)
  extern U64 rmask[64]; // rook mask (outer board edges are trimmed)
  extern U64 squares[64];
  extern U64 diagonals[64];
  extern U64 edges;
  extern U64 corners;

  void load();

}

#endif
