#pragma once
#ifndef SBC_EVALUATE_H
#define SBC_EVALUATE_H

#include "board.h"
#include "material.h"
#include "pawns.h"
#include "definitions.h"

#include <vector>
#ifdef DEBUG
#include <cassert>
#endif

namespace Eval
{
  struct Scores {
    Scores() {};
    int material_sc;
    int pawn_sc;
    int knight_sc[2];
    int bishop_sc[2];
    int rook_sc[2];
    int queen_sc[2];
    int king_sc[2];
    int threat_sc[2];
    int safety_sc[2];
    int mobility_sc[2];
    int space_sc[2];
    int center_sc[2];
    int tempo_sc[2];
    int squares_sc[2];
    int time;
  };

  struct KingSafety {
    U64 attackedSquareBB;
    int attackScore[6]; // pawn, knight, bithop, rook, queen, king
    int numAttackers;
    int kingRingAttackers[2][2][6];
  };

  struct EvalInfo {
    EvalInfo() {};
    int stm;
    GamePhase phase;
    U64 pieces[2];
    U64 weak_enemies[2]; // those pieces which are not defended by a pawn
    U64 all_pieces;
    U64 empty;
    U64 pawns[2];
    U64 all_pawns;
    U64 pinned[2];
    U64 backrank[2];
    int kingsq[2];
    int pinners[2][64];
    int tempoBonus;
    int do_trace;
    int endgame_eval;
    bool RooksOpenFile[2]; // back rank weaknesses
    MaterialEntry * me;
    PawnEntry * pe;
    KingSafety ks[2];
    Scores s;
  };
  extern int evaluate(Board& b);
};

#endif 
