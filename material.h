#pragma once
#ifndef HEDWIG_MATERIAL_H
#define HEDWIG_MATERIAL_H

#include <math.h>
#include "definitions.h"
#include "types.h"

// material hash table for hedwig.  The methods/organization of the code
// follow stockfish conventions

class Board;

struct MaterialEntry
{
	U64 key;
	int value;
	GamePhase game_phase;
};

class MaterialTable
{
public:
	MaterialTable();
	~MaterialTable();

	bool init();
	MaterialEntry * get(Board& b);
	int material_value(int piece, int gamephase);
	void clear();

private:
	size_t table_size;
	U64 nb_elts;
	MaterialEntry * table;
};

// a global pointer to the material hash table entries
extern MaterialTable material; 

#endif
