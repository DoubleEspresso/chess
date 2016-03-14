#pragma once

#ifndef HEDWIG_MAGIC_H
#define HEDWIG_MAGIC_H

#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#include "globals.h"
#include "random.h"

struct MagicTable
{
	U8* index_occ;
	U64* attacks;
	U64 mask;
	U64 magic;
	int shift;
	int offset;
};

extern MagicTable RMagics[SQUARES];
extern MagicTable BMagics[SQUARES];

namespace Magic
{
	// utilities to find/compute the hash multipliers
	void init();
	bool check_magics();

	U64 attack_bm(int p, int s, U64 block);
	U64 next_rand(int bits);

	inline unsigned index(U64 &magic, U64 &mask, U64 &occ, int &shift)
	{
		return unsigned(magic * (mask & occ) >> shift);
	}

	template<Piece p>
	inline U64 attacks(U64 &occ, int s);

	template<>
	inline U64 attacks<ROOK>(U64 &occ, int s)
	{
		unsigned idx = index(RMagics[s].magic, RMagics[s].mask, occ, RMagics[s].shift);
		return RMagics[s].attacks[RMagics[s].index_occ[idx] + RMagics[s].offset];
	}

	template<>
	inline U64 attacks<BISHOP>(U64 &occ, int s)
	{
		unsigned idx = index(BMagics[s].magic, BMagics[s].mask, occ, BMagics[s].shift);
		return BMagics[s].attacks[BMagics[s].index_occ[idx] + BMagics[s].offset];
	}
};

#endif