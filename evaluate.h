#pragma once

#ifndef EVALUATE_H
#define EVALUATE_H

#include "types.h"
#include "utils.h"
#include "position.h"
#include "pawns.h"
#include "material.h"


struct endgame_info {
  bool evaluated_fence;
  bool is_fence;
};

struct einfo {
  pawn_entry * pe;
  material_entry * me;
  endgame_info endgame;
  U64 all_pieces;
  U64 pieces[2];
  U64 weak_pawns[2];
  U64 empty;
};


namespace eval {

  struct parameters {
    const float tempo = 4;
    const float mobility_scaling[5] = { 0.0f, 1.0f, 1.0f, 1.0f, 1.0f };

    const float knight_attks[5] = { 3.0f, 9.0f, 9.45f, 14.4f, 27.3f };
    const float bishop_attks[5] = { 3.0f, 9.0f, 9.45f, 14.4f, 27.3f };
    const float rook_attks[5] = { 1.5f, 4.5f, 4.725f, 7.2f, 13.65f };
    const float queen_attks[5] = { 0.75f, 2.25f, 2.3625f, 3.6f, 6.825f };

  };

  float evaluate(const position& p);

}

#endif
