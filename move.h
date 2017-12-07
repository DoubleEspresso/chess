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
// 
// BIT COMBOS                          DESCRIPTION
// 11                                  quite move 
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

class MoveGenerator
{
private:
	int it, last;
	MoveList list[MAX_MOVES];
	int legal_i[MAX_MOVES];

public:
	MoveGenerator() : it(0), last(0) { }
	MoveGenerator(Board &b) : it(0), last(0) { generate(b, LEGAL); }
	MoveGenerator(Board& b, MoveType mt) : it(0), last(0) { generate(b, mt); }
	MoveGenerator(Board& b, MoveType mt, bool legal) : it(0), last(0)
	{
		if (legal) generate(b, mt);
		else generate_pseudo_legal(b, mt);
	}
	~MoveGenerator() { };

	inline U64 move_pawns(int c, int d, U64& b);
	MoveList * generate(Board &b, MoveType mt);
	template<MoveType> inline void append(U16 m);
	MoveList operator++() { return list[legal_i[it++]]; }
	bool end() { return it == last; }
	U16 move() { return list[legal_i[it]].m; }
	int size() const { return last; }
	template<MoveType> void serialize(U64 &b, const U8& from);
	template<MoveType> void serialize(U64 &b, const Direction& d);
	template<MoveType> void serialize_promotion(U64 &b, const Direction& d);
	MoveList * generate_pawn_moves(Board &b, MoveType mt);
	MoveList * generate_piece_moves(Board &b, MoveType mt);
	MoveList * generate_evasions(Board& board, MoveType mt);
	MoveList * generate_qsearch_mvs(Board& b, MoveType mt, bool checksKing);
	MoveList * generate_pseudo_legal(Board& b, MoveType mt);
	MoveList * generate_legal_castle_moves(Board& b);
};
#endif
