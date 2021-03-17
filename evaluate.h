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
#pragma once

#ifndef EVALUATE_H
#define EVALUATE_H

#include "types.h"
#include "utils.h"
#include "pawns.h"
#include "material.h"
#include "parameter.h"
#include "utils.h"

struct endgame_info {
  bool evaluated_fence;
  bool is_fence;
};

struct einfo {
  pawn_entry * pe;
  material_entry * me;
  endgame_info endgame;
  U64 pawn_holes[2];
  U64 all_pieces;
  U64 pieces[2];
  U64 weak_pawns[2];
  U64 empty;
  U64 kmask[2];
  U64 kattk_points[2];
  U64 central_pawns[2];
  U64 queen_sqs[2];
  U64 white_pawns[2];
  U64 black_pawns[2];

  bool closed_center;
  unsigned kattackers[2][5];  
};


namespace eval {

  float evaluate(const position& p, const float& lazy_margin);

}

namespace eval {
  extern parameters Parameters;
}

#endif
