#pragma once
#ifndef HEDWIG_MOVESELECT_H
#define HEDWIG_MOVESELECT_H
#include <cassert>
#include <string.h>

#include "definitions.h"
#include "move.h"
#include "board.h"
#include "search.h"

struct MoveStats
{
	int history[2][SQUARES][SQUARES];
	U16 countermoves[SQUARES][SQUARES];

	void update(Board& b, U16& m, U16& last, Node* stack, int d, int eval, U16* quiets);

	int score(U16& m, int c)
	{
		int f = get_from(m);
		int t = get_to(m);
		return history[c][f][t];
	}
	void clear()
	{
		for (int c = 0; c < 2; ++c)
			for (int i = 0; i < SQUARES; ++i)
				for (int j = 0; j < SQUARES; ++j)
					history[c][i][j] = NINF - 1;

		for (int s1 = 0; s1 < SQUARES; ++s1)
			for (int s2 = 0; s2 < SQUARES; ++s2)
				countermoves[s1][s2] = MOVE_NONE;

	}
};

enum SearchType { Probcut, MainSearch, QsearchCaptures, QsearchChecks, QsearchEvasions };

enum Phase {
        HashMove, MateKiller1, MateKiller2, GoodCaptures,
	Killer1, Killer2, BadCaptures, Quiet, End
};

class MoveSelect
{
	int c_sz, stored_csz;
	int q_sz, stored_qsz;
	int phase;
	MoveList captures[MAX_MOVES];
	MoveList quiets[MAX_MOVES];
	//MoveList checks[MAX_MOVES];
	//MoveList promotions[MAX_MOVES];
	//MoveList evasions[MAX_MOVES];
	MoveStats * statistics;
	SearchType type;
	bool genChecks;

public:
	MoveSelect(MoveStats& stats, SearchType t) :
		c_sz(0), stored_csz(0), q_sz(0),
		stored_qsz(0), phase(HashMove), statistics(0), type(t), genChecks(false)
	{
		// fixme...
		for (int j = 0; j < MAX_MOVES; ++j)
		{
			captures[j].score = NINF - 1; captures[j].m = MOVE_NONE;
			quiets[j].score = NINF - 1; quiets[j].m = MOVE_NONE;
		}
		statistics = &stats;
	}

	MoveSelect(MoveStats& stats, SearchType t, bool gChecks) :
		c_sz(0), stored_csz(0), q_sz(0),
		stored_qsz(0), phase(HashMove), statistics(0), type(t), genChecks(gChecks)
	{
		// fixme...
		for (int j = 0; j < MAX_MOVES; ++j)
		{
			captures[j].score = NINF - 1; captures[j].m = MOVE_NONE;
			quiets[j].score = NINF - 1; quiets[j].m = MOVE_NONE;
		}
		statistics = &stats;
	}


	~MoveSelect() {};

	void sort(MoveList * ml, int length);
	bool nextmove(Board &b, Node * stack, U16& ttm, U16& out, bool split);

	// dbg
	MoveList * get_quites() { return quiets; }
	int qsize() { return stored_qsz; }
	int csize() { return stored_csz; }
	MoveList * get_captures() { return captures; }
	void print_list();
	void load_and_sort(MoveGenerator& mvs, Board& b, U16& ttm, Node * stack, MoveType movetype);
};

#endif
