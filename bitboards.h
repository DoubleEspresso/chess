/*
-----------------------------------------------------------------------------
This source file is part of the Havoc chess engine
Copyright (c) 2020 Minniesoft
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
# pragma once

#ifndef BITBOARDS_H
#define BITBOARDS_H

#include "types.h"
#include "utils.h"

namespace bitboards {

  extern U64 row[8];
  extern U64 col[8];
  extern U64 pawnmask[2]; // 2nd - 6th rank mask for pawns (to exclude promotion candidates)
  extern U64 pawnmaskleft[2]; // 2nd - 6th rank mask for pawn captures
  extern U64 pawnmaskright[2]; // 2nd - 6th rank mask for pawn captures
  extern U64 pattks[2][64]; // step attacks for the pawns
  extern U64 nmask[64]; // step attacks for the knight
  extern U64 kmask[64]; // step attacks for the king
  extern U64 kchecks[64];
  extern U64 kflanks[8]; // 3 rows of squares (including king square) for pawn cover detection
  extern U64 kzone[64];
  extern U64 bmask[64]; // bishop mask (outer board edges are trimmed)
  extern U64 rmask[64]; // rook mask (outer board edges are trimmed)
  extern U64 squares[64];
  extern U64 battks[64];
  extern U64 rattks[64];
  extern U64 between[64][64]; // bits set between 2 squares that are aligned
  extern U64 edges;
  extern U64 corners;
  extern U64 small_center_mask;
  extern U64 big_center_mask;
  extern U64 pawn_majority_masks[3];
  extern U64 passpawn_mask[2][64];
  extern U64 neighbor_cols[8];
  extern U64 colored_sqs[2];
  extern unsigned reductions[2][2][64][64]; // max-depth = 64

  void load();

}

#endif
