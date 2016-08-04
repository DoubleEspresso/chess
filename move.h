#pragma once
#ifndef HEDWIG_MOVE_H
#define HEDWIG_MOVE_H

#include <stdio.h>

#include "definitions.h"
#include "types.h"
#include "globals.h"

class Board;


//---------------------------------------------
// Move Encoding:
// We store move information in a 16-bit word, 
// using the standard 'from'-'to' representation.
// The 16-bit word is broken down as follows,
//
// SET BITS                            DESCRIPTION
// 0-5                                 from sq (0-63)
// 6-11                                to sq   (0-63)
// 12-16                               move data
// 
// this leaves bits 12-15 as move_data flags
// we enumerate the possible bit combinations
// here
// 
// BIT COMBOS                          DESCRIPTION
// 11                                  quite move (no check?)
// 12                                  normal capture
// 13                                  ep capture
// 14                                  quite move (gives check)
// PROMOTIONS -- CAPTURES
// 5                                   p.c. to knight                               
// 6                                   p.c. to bishop
// 7                                   p.c. to rook
// 8                                   p.c. to queen
// CASTLES
// 9                                   castle k.s.
// 10                                  castle q.s.
// PROMOTIONS -- QUITES
// 1                                   promotion to knight
// 2                                   promotion to bishop
// 3                                   promotion to rook
// 4                                   promotion to queen
//---------------------------------------------

// movelist structure is used for move array storage 
// and contains a score used exclusively for move ordering purposes
struct MoveList
{
  U16 m;
  int score;
};

inline bool operator <(const MoveList& x, const MoveList& y) 
{
  return x.score < y.score;
}

inline bool operator >(const MoveList& x, const MoveList& y)
{
  return x.score > y.score;
}

class MoveGenerator
{
 public:
 MoveGenerator() : it(0), last(0)
    {
    }
 MoveGenerator(Board &b) : it(0), last(0)
    {
      generate(b, LEGAL);
    }
 MoveGenerator(Board& b, MoveType mt) : it(0), last(0)
    {
      if (mt == CAPTURE_CHECKS) generate_qsearch_caps_checks(b);
      else generate(b, mt);
    }
  
  // this c'tor is meant for calls from qsearch only!!
 MoveGenerator(Board& b, int depth, bool inCheck) : it(0), last(0)
    {
      if (inCheck) generate_qsearch_mvs(b); // evasions
      else if (depth >= 0) generate_qsearch_caps_checks(b);
      //else generate_qsearch_caps_checks(b);
      else generate(b, CAPTURE);
    }
  ~MoveGenerator() { };
  
  inline U64 move_pawns(int c, int d, U64& b);
  MoveList * generate(Board &b, MoveType mt);
  
  void append_mv(U16 m) { list[last++].m = m; }
  template<MoveType> inline void append(U16 m);
  MoveList operator++() { return list[legal_i[it++]]; }
  bool end() { return it == last; }
  U16 move() { return list[legal_i[it]].m; }
  int size() const { return last; }
  
  template<MoveType> void serialize(U64 &b, int from);
  template<MoveType> void serialize(U64 &b, Direction d);
  template<MoveType> void serialize_promotion(U64 &b, Direction d);
  MoveList * generate_pawn_moves(Board &b, MoveType mt);
  MoveList * generate_piece_moves(Board &b, MoveType mt);
  MoveList * generate_piece_evasions(Board& board);
  MoveList * generate_legal_caps(Board& board);
  MoveList * generate_qsearch_mvs(Board& b);
  MoveList * generate_qsearch_caps_checks(Board& b);
  U64 generate_candidate_checkers(Board& b);
  MoveList * generate_legal_castle_moves(Board& b);
  inline bool is_legal_ep(int from, int to, int ks, int ec, Board& b);
  inline bool is_legal_km(int ks, int to, int ec, Board& b, int type);
  
 private:
  int it;
  int last;
  MoveList list[MAX_MOVES];
  int legal_i[MAX_MOVES];
};
#endif
