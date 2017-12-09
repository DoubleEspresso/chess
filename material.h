#pragma once
#ifndef SBC_MATERIAL_H
#define SBC_MATERIAL_H

#include <math.h>
#include "definitions.h"
#include "types.h"

// material hash table for sbchess.  The methods/organization of the code
// follow stockfish conventions

class Board;

struct MaterialEntry
{
  U64 key;
  int value;
  GamePhase game_phase;
  EndgameType endgame_type;
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

extern MaterialTable material; 

#endif
