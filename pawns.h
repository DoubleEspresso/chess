#pragma once
#ifndef SBC_PAWNS_H
#define SBC_PAWNS_H

#include "board.h"

struct PawnEntry
{
  U64 key;
  int value;

  U64 doubledPawns[2];
  U64 isolatedPawns[2];
  U64 backwardPawns[2];
  U64 chainPawns[2];
  U64 passedPawns[2];
  U64 kingPawns[2];
  U64 attacks[2];
  U64 undefended[2];
  U64 chainBase[2];
  U64 semiOpen[2];
  U64 pawnChainTips[2];
  int blockedCenter;
  int kSidePawnStorm;
  int qSidePawnStorm;
};

class PawnTable
{
 private:
  U64 nb_elts;
  size_t sz_kb;
  PawnEntry * table;
  
 public:
  PawnTable();
  ~PawnTable();

  bool init();
  void clear();

  PawnEntry * get(Board& b, GamePhase gp);
  int eval(Board& b, Color c, GamePhase gp, int idx);  
  void debug(PawnEntry& e);
};

extern PawnTable pawnTable;

#endif
