#pragma once
#ifndef SBC_SQS_H
#define SBC_SQS_H

#include <cmath>
#include "definitions.h"

namespace {

  static const int square_scores2[2][6][SQUARES] =
    {
      {
	//pawns
	{
	  0, 0, 0, 0, 0, 0, 0, 0,
	  1, 2, 10, -6, -6, 10, 2, 1,
	  1, 3, 10, -2, -2, 10, 3, 1,
	  1, 0, 20, 30, 30, 20, 0, 1,
	  5, 20, 20, 30, 30, 30, 20, 5,
	  5, 20, 20, 30, 30, 30, 20, 5,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0
	},
	//knights
	{
	  -40, -20, -20, -20, -20, -20, -20, -40,
	  -20, 0, 0, 0, 0, 0, 0, -20,
	  -20, 0, 15, 10, 10, 15, 0, -20,
	  -20, 0, 15, 20, 20, 15, 0, -20,
	  -20, 0, 35, 35, 35, 35, 0, -20,
	  -20, 0, 45, 45, 45, 45, 0, -20,
	  -20, 0, 0, 0, 0, 0, 0, -20,
	  -40, -20, -20, -20, -20, -20, -20, -40
	},
	//bishops
	{
	  -20,-10,-10,-10,-10,-10,-10,-20,
	  -10,  0,  0,  0,  0,  0,  0,-10,
	  -10,  0,  1, 0, 0,  1,  0,-10,
	  -10,  2,  2, 10, 10,  2,  2,-10,
	  -10,  0, 10, 10, 10, 10,  0,-10,
	  -10, 10, 10, 10, 10, 10, 10,-10,
	  -10,  0,  0,  0,  0,  0,  0,-10,
	  -20,-10,-10,-10,-10,-10,-10,-20
	},
	//rooks
	{
	  0, 0, 2, 4, 4, 2, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  8, 8, 8, 8, 8, 8, 8, 8,
	  0, 0, 0, 0, 0, 0, 0, 0
	},
	//queens
	{
	  0, 0, 2, 4, 4, 2, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 4, 10, 10, 4, 2, 0,
	  0, 2, 4, 10, 10, 4, 2, 0,
	  0, 2, 4, 8, 8, 4, 2, 0,
	  0, 2, 2, 4, 4, 2, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0
	},
	//kings
	{
	  0, 0, 8, -4, -4, -4, 8, 0,
	  -4, -4, -4, -4, -4, -4, -4, -4,
	  -6, -6, -6, -6, -6, -6, -6, -6,
	  -8, -8, -8, -8, -8, -8, -8, -8,
	  -10, -10, -10, -10, -10, -10, -10, -10,
	  -10, -10, -10, -10, -10, -10, -10, -10,
	  -10, -10, -10, -10, -10, -10, -10, -10,
	  -10, -10, -10, -10, -10, -10, -10, -10
	}
      },
      //endgame
      {
	//pawns
	{
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  10, 10, 10, 10, 10, 10, 10, 10,
	  15, 15, 25, 25, 25, 25, 15, 15,
	  25, 25, 45, 45, 45, 45, 25, 25,
	  30, 30, 50, 55, 55, 50, 30, 30,
	  35, 35, 55, 60, 60, 55, 35, 35,
	  40, 45, 65, 70, 70, 65, 45, 40
	},
	//knights
	{
	  -40, -20, -20, -20, -20, -20, -20, -40,
	  -20, 0, 0, 0, 0, 0, 0, -20,
	  -20, 0, 10, 10, 10, 10, 0, -20,
	  -20, 0, 10, 20, 20, 10, 0, -20,
	  -20, 0, 35, 35, 35, 35, 0, -20,
	  -20, 0, 40, 40, 40, 40, 0, -20,
	  -20, 0, 0, 0, 0, 0, 0, -20,
	  -40, -20, -20, -20, -20, -20, -20, -40
	},
	//bishops
	{
	  -20,-10,-10,-10,-10,-10,-10,-20,
	  -10,  0,  0,  0,  0,  0,  0,-10,
	  -10,  0,  1, 0, 0,  1,  0,-10,
	  -10,  2,  2, 10, 10,  2,  2,-10,
	  -10,  0, 10, 10, 10, 10,  0,-10,
	  -10, 10, 10, 10, 10, 10, 10,-10,
	  -10,  0,  0,  0,  0,  0,  0,-10,
	  -20,-10,-10,-10,-10,-10,-10,-20
	},
	//rooks
	{
	  0, 0, 2, 4, 4, 2, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  8, 8, 8, 8, 8, 8, 8, 8,
	  0, 0, 0, 0, 0, 0, 0, 0
	},
	//whitequeens
	{
	  0, 0, 2, 4, 4, 2, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 4, 8, 8, 4, 2, 0,
	  0, 2, 4, 8, 8, 4, 2, 0,
	  0, 2, 4, 8, 8, 4, 2, 0,
	  0, 2, 2, 4, 4, 2, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0
	},
	//king
	{
	  0, 0, 0, 0, 0, 0, 0, 0,
	  2, 2, 4, 4, 4, 4, 2, 2,
	  6, 6, 12, 12, 12, 12, 6, 6,
	  12, 12, 24, 24, 24, 24, 12, 12,
	  16, 16, 32, 32, 32, 32, 16, 16,
	  20, 20, 40, 40, 40, 40, 20, 20,
	  20, 20, 40, 40, 40, 40, 20, 20,
	  20, 20, 40, 40, 40, 40, 20, 20
	}
      }
    };


  template<Color c, Piece p>
    inline int square_score(int phase, int square)
  {
    if (c == BLACK)
      {
	square = 56 - 8 * ROW(square) + COL(square);
      }
    return square_scores2[phase][p][square];
  }

  // non-templated version (drops the const color and piece requirements)
  inline int square_score(int c, int p, int phase, int square)
  {
    if (c == BLACK)
      {
	square = 56 - 8 * ROW(square) + COL(square);
      }
    return square_scores2[phase][p][square];
  }


} // end namespace

#endif
