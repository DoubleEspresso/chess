#include "bitboards.h"
#include "bits.h"

namespace bitboards {
  U64 row[8];
  U64 col[8];
  U64 color[2]; // white/black squares
  U64 bmask[64]; // bishop mask (outer board edges are trimmed)
  U64 pawnmask[2]; // 2nd - 6th rank mask for pawns (to exclude promotion candidates)
  U64 pawnmaskleft[2];
  U64 pawnmaskright[2];
  U64 rmask[64]; // rook mask (outer board edges are trimmed)
  U64 squares[64];
  U64 diagonals[64];
  U64 edges;
  U64 corners;
}

void bitboards::load() {

  std::vector<std::vector<int>> steps = 
    { 
      { }, // pawn 
      { }, // knight
      { -7, -9, 7, 9 }, // bishop
      { -1, 1, 8, -8 }, // rook
      { }, // queen
      { }  // king
    };
  
  for (Square s = A1; s <= H8; ++s) squares[s] = (1ULL << s);

  // row/col masks
  for (Row r = r1; r <= r8; ++r) {
    U64 rw = 0ULL; U64 cl = 0ULL;    
    for (Col c = A; c <= H; ++c) {
      cl |= squares[c * 8 + r];
      rw |= squares[r * 8 + c];
    }
    row[r] = rw;
    col[r] = cl;
  }

  // helpful definitions for board corners/edges
  edges = row[r1] | col[A] | row[r8] | col[H];
  corners = squares[A1] | squares[H1] | squares[H8] | squares[A8];

  // pawn masks for captures/promotions
  pawnmask[white] = row[r2] | row[r3] | row[r4] | row[r5] | row[r6];
  pawnmask[black] = row[r3] | row[r4] | row[r5] | row[r6] | row[r7];
  for (Color color = white; color <= black; ++color) {      
    pawnmaskleft[color] = 0ULL;
    pawnmaskright[color] = 0ULL;    
    for (int r = (color == white ? 1 : 2); r <= (color == white ? 5 : 6); ++r) { 
      for (int c = 0; c <= 6; ++c) pawnmaskleft[color] |= squares[r*8+c];
      for (int c = 1; c <= 7; ++c) pawnmaskright[color] |= squares[r*8+c];
    }
  }

    
  
  for (Square s = A1; s <= H8; ++s) {

    // bishop diagonal masks (outer bits trimmed)
    U64 bm = 0ULL;
    U64 trim = 0ULL;
    for (auto& step : steps[Piece::bishop]) {
      int j = 0;
      while (true) {
        int to = s + (j++) * step;
        if (util::on_board(to) && util::on_diagonal(s, to)) bm |= squares[to];
        else break;
      }
    }
    trim = squares[s] | (bm & edges);
    bm ^= trim;
    bmask[s] = bm;

    // rook masks (outer-bits trimmed)
    bm = (row[util::row(s)] | col[util::col(s)]);
    trim = squares[s] | squares[8*util::row(s)] | squares[8*util::row(s)+7] |
      squares[util::col(s)] | squares[util::col(s) + 56];
    bm ^= trim; 
    rmask[s] = bm;
  }   
}

