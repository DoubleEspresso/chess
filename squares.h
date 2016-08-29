#pragma once
#ifndef HEDWIG_SQS_H
#define HEDWIG_SQS_H

#include <cmath>

#include "definitions.h"


#define S(x,y) pick_table_score(x,y)

namespace {

  inline int pick_table_score(int x, int y)
  {
    return (x + 100 * y);
  }

  static const int territory_scores[SQUARES] = 
    {
      S(1,1), S(1,1), S(1,1), S(1,1), S(1,1), S(1,1), S(1,1), S(1,1),
      S(1,1), S(1,1), S(1,1), S(1,1), S(1,1), S(1,1), S(1,1), S(1,1),
      S(1,1), S(1,1), S(2,1), S(2,1), S(2,1), S(2,1), S(1,1), S(1,1),
      S(1,1), S(1,1), S(8,2), S(8,4), S(8,4), S(8,2), S(1,1), S(1,1),
      S(1,1), S(1,1), S(4,4), S(4,4), S(4,4), S(4,4), S(1,1), S(1,1),
      S(1,1), S(1,1), S(2,4), S(2,4), S(2,4), S(2,4), S(1,1), S(1,1),
      S(1,1), S(1,1), S(1,1), S(1,1), S(1,1), S(1,1), S(1,1), S(1,1),
      S(1,1), S(1,1), S(1,1), S(1,1), S(1,1), S(1,1), S(1,1), S(1,1)
    };

  static const int square_scores[6][SQUARES] =
    {
      // white pawns
      {
	S(0,0), S(0,0), S(0,0), S(0,0), S(0,0), S(0,0), S(0,0), S(0,0),
	S(10,5), S(10,5), S(10,5), S(5,2), S(5,2), S(10,5), S(10,5), S(10,5),
	S(10,10), S(10,10), S(10,10), S(5,2), S(5,2), S(10,10), S(10,10), S(10,10),
	S(10,10), S(10,10), S(10,10), S(20,25), S(20,25), S(10,10), S(10,10), S(10,10),
	S(20,25), S(20,25), S(40,45), S(40,45), S(40,45), S(40,45), S(20,25), S(20,25),
	S(30,85), S(30,85), S(50,85), S(50,85), S(50,85), S(30,85), S(30,85), S(30,85),
	S(90,100), S(90,100), S(90,100), S(90,100), S(90,100), S(90,100), S(90,100), S(90,100),
	S(90,100), S(90,100), S(90,100), S(90,100), S(90,100), S(90,100), S(90,100), S(90,100),
      },
      // white knights
      {
	S(-10,-10), S(-10,-10), S(-10,-10), S(-10,-10), S(-10,-10), S(-10,-10), S(-10,-10), S(-10,-10),
	S(-10,-10), S(0,0)    , S(0,0)    , S(4,4)    , S(4,4)    , S(0,0)    , S(0,0)    , S(-10,-10),
	S(-10,-10), S(0,0)    , S(8,8)    , S(4,4)    , S(4,4)    , S(8,8)    , S(0,0)    , S(-10,-10),
	S(-10,-10), S(6,6)    , S(10,15)  , S(10,15)  , S(10,15)  , S(10,15)  , S(6,6)    , S(-10,-10),
	S(-10,-10), S(15,15)  , S(15,15)  , S(15,15)  , S(15,15)  , S(15,15)  , S(15,15)  , S(-10,-10),
	S(-10,-10), S(20,20)  , S(20,20)  , S(20,20)  , S(20,20)  , S(20,20)  , S(20,20)  , S(-10,-10),
	S(-10,-10), S(0,0)    , S(0,0)    , S(0,0)    , S(0,0)    , S(0,0)    , S(0,0)    , S(-10,-10),
	S(-10,-10), S(-10,-10), S(-10,-10), S(-10,-10), S(-10,-10), S(-10,-10), S(-10,-10), S(-10,-10)
      },
      // white bishops
      {
	S(0,0), S(0,0), S(0,0), S(0,0), S(0,0), S(0,0), S(0,0), S(0,0),
	S(0,0), S(2,2), S(2,2), S(2,2), S(2,2), S(2,2), S(2,2), S(0,0),
	S(0,0), S(2,2), S(2,2), S(2,2), S(2,2), S(2,2), S(2,2), S(0,0),
	S(0,0), S(4,4), S(6,6), S(6,6), S(6,2), S(6,6), S(4,4), S(0,0),
	S(0,0), S(8,8), S(10,10), S(10,10), S(10,10), S(10,10), S(8,8), S(0,0),
	S(0,0), S(6,6), S(8,8), S(8,8), S(8,8), S(8,8), S(6,6), S(0,0),
	S(0,0), S(4,4), S(6,6), S(6,6), S(6,2), S(6,6), S(4,4), S(0,0),
	S(0,0), S(0,0), S(0,0), S(0,0), S(0,0), S(0,0), S(0,0), S(0,0)
      },
      // white rooks
      {
	S(0,0), S(0,0), S(2,2), S(4,4), S(4,4), S(2,2), S(0,0), S(0,0),
	S(0,0), S(0,0), S(2,2), S(4,4), S(4,4), S(2,2), S(0,0), S(0,0),
	S(0,0), S(0,0), S(2,2), S(4,4), S(4,4), S(2,2), S(0,0), S(0,0),
	S(0,0), S(0,0), S(4,4), S(4,4), S(4,4), S(4,4), S(0,0), S(0,0),
	S(0,0), S(0,0), S(6,6), S(6,6), S(6,6), S(6,6), S(0,0), S(0,0),
	S(0,0), S(0,0), S(8,8), S(8,8), S(8,8), S(8,8), S(0,0), S(0,0),
	S(12,12), S(12,12), S(12,12), S(12,12), S(12,12), S(12,12), S(12,12), S(12,12),
	S(12,12), S(12,12), S(12,12), S(12,12), S(12,12), S(12,12), S(12,12), S(12,12)
      },
      // white queens
      {
	S(0,0), S(0,0), S(2,2), S(4,4), S(4,4), S(2,2), S(0,0), S(0,0),
	S(0,0), S(0,0), S(2,2), S(4,4), S(4,4), S(2,2), S(0,0), S(0,0),
	S(0,0), S(0,0), S(2,2), S(4,4), S(4,4), S(2,2), S(0,0), S(0,0),
	S(0,0), S(0,0), S(4,4), S(8,8), S(8,8), S(4,4), S(2,2), S(0,0),
	S(0,0), S(2,2), S(4,4), S(8,8), S(8,8), S(4,4), S(2,2), S(0,0),
	S(0,0), S(4,4), S(6,6), S(8,8), S(8,8), S(6,6), S(4,4), S(0,0),
	S(0,0), S(2,2), S(6,6), S(8,8), S(8,8), S(6,6), S(2,2), S(0,0),
	S(0,0), S(2,2), S(6,6), S(8,8), S(8,8), S(6,6), S(2,2), S(0,0),
      },
      // white king
      {
	S(0,0)    , S(0,0)    , S(8,8)    , S(-4,-4)  , S(-4,-4)  , S(-4,-4)  , S(8,8)    , S(0,0),
	S(-4,0)  , S(-4,4)  , S(-4,5)  , S(-4,6)  , S(-4,6)  , S(-4,5)  , S(-4,4)  , S(-4,0)  ,
	S(-6,0)  , S(-6,6)  , S(-6,7)  , S(-6,8)  , S(-6,8)  , S(-6,7)  , S(-6,6)  , S(-6,0)  ,
	S(-8,0)  , S(-8,8)  , S(-8,9)  , S(-8,10)  , S(-8,10)  , S(-8,9)  , S(-8,8)  , S(-8,0)  ,
	S(-10,0), S(-10,10), S(-10,11), S(-10,12), S(-10,12), S(-10,11), S(-10,10), S(-10,0),
	S(-10,0), S(-10,12), S(-10,13), S(-10,14), S(-10,14), S(-10,13), S(-10,12), S(-10,0),
	S(-10,0), S(-10,12), S(-10,13), S(-10,14), S(-10,14), S(-10,13), S(-10,12), S(-10,0),
	S(-10,0), S(-10,10), S(-10,10), S(-10,10), S(-10,10), S(-10,10), S(-10,10), S(-10,0)
      }
    };

  inline float round(float number)
  {
    return number < 0.0 ? ceil(number - 0.5) : floor(number + 0.5);
  }

  template<Color c>
    int territory_score(int phase, int square)
    {
    if (c == BLACK)
      {
	
	int col = COL(square);
	int row = 7 - ROW(square);
	square = 8 * row + col;
      }
    int ss = territory_scores[square];
    int eg = ss / 100;
    int mg = ss - 100 * round( (float) ss / (float) 100);
    return (phase == MIDDLE_GAME ? mg : eg); 
  }

  template<Color c, Piece p>
    int square_score(int phase, int square)
  {
    if (c == BLACK)
      {
		  
	int col = COL(square);
	int row = 7 - ROW(square);
	square = 8 * row + col;
      }
    int ss = square_scores[p][square];
	int eg = ss / 100;
	//int rp = (eg <= 0 ? floor(ss / 100 + 0.5) : floor(ss / 100 + 0.5)); 
	//printf("rp = %d\n", rp);
	int mg = ss - 100 * round( (float) ss / (float) 100);
	//printf("1. c=%d, p=%d, sq=%d, phase=%d, ss=%d, eg=%d, mg=%d\n", c, p, square, phase, ss, eg, mg);
    return (phase == MIDDLE_GAME ? mg : eg); 
  }

  // non-templated version (drops the const color and piece requirements)
inline int square_score(int c, int p, int phase, int square)
  {
    if (c == BLACK)
      {
	int col = COL(square);
	int row = 7 - ROW(square);
	square = 8 * row + col;
      }
	int ss = square_scores[p][square];	
	int eg = ss / 100;
	//int rp = (eg <= 0 ? floor(ss / 100 + 0.5) : floor(ss / 100 + 0.5)); 
	int mg = ss - 100 * round( (float) ss / (float) 100);
	//printf("2. c=%d, p=%d, sq=%d, phase=%d, ss=%d, eg=%d, mg=%d\n", c, p, square, phase, ss, eg, mg);
    return (phase == MIDDLE_GAME ? mg : eg);
  }


}; // end namespace

#undef S

#endif
