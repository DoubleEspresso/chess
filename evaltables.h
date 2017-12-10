#pragma once
#ifndef SBC_EVALTABLES_H
#define SBC_EVALTABLES_H

namespace { 
  int PinPenalty[5][5] = // [pinning piece][pinned piece]
    {
      {}, // pinned by pawn
      {}, // pinned by knight
      {0,1,0,2,3}, // pinned by bishop
      {0,0,0,0,1}, // pinned by rook
      {0,0,0,0,0} // pinned by queen	    
    };
  int DevelopmentBonus[5] = { 0, 8, 6, 0, 0 }; // pawn, knight, bishop, rook, queen
  int TrappedPenalty[5] = { 0, 2, 3, 4, 5 }; // pawn, knight, bishop, rook, queen
  int BishopCenterBonus[2] = { 4 , 6 };
  int DoubleBishopBonus[2] = { 16 , 22 };
  int RookOpenFileBonus[2] = { 6, 8 };
  int AttackBonus[2][5][6] =
    {
      {
	// pawn, knight, bishop, rook, queen, king
	{ 0, 5, 6, 7, 8, 9 },  // pawn attcks mg
	{ 0, 2, 3, 7, 11, 12 },  // knight attcks mg
	{ 0, 0, 2, 6, 11, 12 },   // bishop attcks mg
	{ 0, 0, 0, 2, 6, 12 },    // rook attcks mg
	{ 0, 0, 0, 2, 3, 12 },    // queen attcks mg
      },
      {
	// pawn, knight, bishop, rook, queen, king
	{ 0, 5, 6, 7, 8, 9 },  // pawn attcks eg
	{ 0, 2, 3, 7, 11, 12 },  // knight attcks eg
	{ 0, 0, 2, 6, 11, 12 },   // bishop attcks eg
	{ 0, 0, 0, 2, 6, 12 },    // rook attcks eg
	{ 0, 0, 0, 2, 3, 12 },    // queen attcks eg
      }
    };
  int KingAttackBonus[6] = { 6, 14, 10, 12, 14, 24 }; // pawn, knight, bishop, rook, queen, king
  int KingExposureBonus[2] = { 2, 0 };
  int CastleBonus[2] = { 6, 1 };
  int PawnLeverScore[64] =
    {
      1, 2, 3, 4, 4, 3, 2, 1,
      1, 2, 3, 4, 4, 3, 2, 1,
      1, 2, 3, 4, 4, 3, 2, 1,
      1, 2, 3, 4, 4, 3, 2, 1,
      1, 2, 3, 4, 4, 3, 2, 1,
      1, 2, 3, 4, 4, 3, 2, 1,
      1, 2, 3, 4, 4, 3, 2, 1,
      1, 2, 3, 4, 4, 3, 2, 1
    };
  
    /*
    int KnightOutpostBonus[2][64] =
    {
    // white
    {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 1, 1, 1, 0, 0,
    0, 1, 2, 3, 3, 2, 1, 0,
    0, 2, 3, 4, 4, 3, 2, 0,
    1, 3, 4, 5, 5, 4, 3, 1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
    },
    // black
    {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 3, 4, 5, 5, 4, 3, 1,
    0, 2, 3, 4, 4, 3, 2, 0,
    0, 1, 2, 3, 3, 2, 1, 0,
    0, 0, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
    }
    };
  */  
};

#endif
