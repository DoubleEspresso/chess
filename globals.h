#pragma once

#ifndef SBC_GLOBALS_H
#define SBC_GLOBALS_H

#include "types.h"
#include "definitions.h"
#include "utils.h"
#include "bits.h"

namespace Globals
{
  // board geometry bitmaps
  extern U64 RowBB[ROWS];
  extern U64 ColBB[COLS];
  extern U64 ColorBB[COLORS];               // colored squares (white or black)
  extern U64 NeighborColsBB[COLS];          // returns the neighbor columns (for isolated pawns)
  extern U64 NeighborRowsBB[ROWS];          // king safety
  extern U64 PassedPawnBB[COLORS][SQUARES]; // in front of and each side of the pawn
  extern U64 BetweenBB[SQUARES][SQUARES];   // bitmap of squares aligned between 2 squares, used for pins
  extern U64 SquareBB[SQUARES+2];             // store each square as a U64 for fast access
  extern U64 SmallCenterBB;
  extern U64 BigCenterBB;
  extern U64 CornersBB;
  extern U64 LongDiagonalsBB;
  extern U64 BoardEdgesBB;
  extern U64 PawnMaskLeft[2];
  extern U64 PawnMaskRight[2];
  extern U64 PawnMaskAll[2];

  // evaluation/utility bitmaps
  extern U64 PawnAttacksBB[COLORS][SQUARES];          // the pawn attacks (by color)
  extern U64 AttacksBB[PIECES-1][SQUARES];            // knight,bishop,rook,queen,king
  extern U64 BishopMask[SQUARES];                     // bishop mask (outer board edges are trimmed)
  extern U64 RookMask[SQUARES];                       // rook mask (outer board edges are trimmed)
  extern U64 QueenMask[SQUARES];                      // queen mask (outer board edges are trimmed)
  extern U64 CastleMask[2][2];                        // determine castle state of each side
  extern U64 KingSafetyBB[COLORS][SQUARES];           // generic array around king (for pawn shelter computation)
  extern U64 KingVisionBB[COLORS][PIECES-1][SQUARES]; // pseudo attack bitboards centered at each king-square
  extern U64 SpaceBehindBB[COLORS][SQUARES];          // all space behind a square
  extern U64 SpaceInFrontBB[COLORS][SQUARES];         // all space in front of a square
  extern U64 ColoredSquaresBB[COLORS];
  extern U64 CenterMaskBB;
  extern U64 kSidePawnStormBB;
  extern U64 qSidePawnStormBB;
  extern int SearchReductions[2][2][MAXDEPTH][MAXDEPTH]; // search reduction table (same as in stockfish)
	
  bool init();
  U64 PseudoAttacksBB(int piece, int square);
}


#endif
