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
#include <vector>

#include "material.h"
#include "types.h"
#include "utils.h"
#include "bitboards.h"


material_table mtable;


int16 evaluate(const position& p, material_entry& e);


inline size_t pow2(size_t x) {
  return x <= 2 ? x : pow2(x >> 1) >> 1;
}


material_table::material_table() : sz_mb(0), count(0) {
  init();
}

void material_table::init() {
  sz_mb = 50 * 1024;
  count = 1024 * sz_mb / sizeof(material_entry);
  if (count < 1024) count = 1024;
  entries = std::unique_ptr<material_entry[]>(new material_entry[count]());
}

void material_table::clear() {
  memset(entries.get(), 0, count * sizeof(material_entry));
}


material_entry * material_table::fetch(const position& p) {
  U64 k = p.material_key();
  unsigned idx = k & (count - 1);
  if (entries[idx].key == k) {
    return &entries[idx];
  }
  else {
    entries[idx] = {};
    entries[idx].key = k;
    entries[idx].score = evaluate(p, entries[idx]);
    return &entries[idx];
  }
}




int16 evaluate(const position& p, material_entry& e) {

  std::vector<float> material_vals{ 0.0f, 300.0f, 315.0f, 480.0f, 910.0f };
  std::vector<int> sign{ 1, -1 };
  const std::vector<Piece> pieces{ knight, bishop, rook, queen }; // pawns handled in pawns.cpp

  // pawn count adjustments for the rook and knight
  // 1. the knight becomes less valuable as pawns dissapear
  // 2. the rook becomes more valuable as pawns dissapear
  // 3. adjustment ~6 pts / pawn so that 16*6 = 96 max adjustment
  {
    const float pawn_adjustment = 2.0;
    U64 wpawns = p.get_pieces<white, pawn>();
    U64 bpawns = p.get_pieces<black, pawn>();
    int total_pawns = bits::count(wpawns) + bits::count(bpawns);
    int minor_pawn_adjust = pawn_adjustment * total_pawns;
    material_vals[knight] -= minor_pawn_adjust;
    material_vals[rook] += minor_pawn_adjust;
  }

  int16 score = 0;
  unsigned total = 0;
  e.endgame = EndgameType::none;

  for (Color c = white; c <= black; ++c) {
    for (const auto& piece : pieces) {
      int n = p.number_of(c, piece);
      e.number[piece] += n;
      score += sign[c] * n * material_vals[piece];
      total += n;
    }
  }

  // encoding endgame type if applicable
  // see types.h for enumeration of different endgame types
  if (total <= 2) {
    U8 endgame_encoding = U8(0);
    int count = 0;
    // see types.h for encoding notes
    for (const auto& piece : pieces) {
      int i = int(piece - 1);
      if (e.number[piece] == 2) {
        // two pieces of the same type
        endgame_encoding |= (U8(1) << i);
        endgame_encoding |= (U8(1) << 4 + i);
      }
      else if (e.number[piece] == 1) {
        endgame_encoding |= count == 0 ? (U8(1) << i) : 
          (U8(1) << (4 + i));
        ++count;
      }
    }
    e.endgame = EndgameType(endgame_encoding);
  }

  // endgame linear interpolation coefficient
  // computed from piece count - excludes pawns
  // endgame_coeff of 0 --> piece count = 14
  // endgame_coeff of 1 --> piece count = 2
  // coeff = -1/12*total + 7/6
  e.endgame_coeff = std::min(-0.083333f * total + 1.16667f, 1.0f);

  return score;
}
