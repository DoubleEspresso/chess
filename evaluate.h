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
  U64 kmask[2];
  U64 kattk_points[2];
  unsigned kattackers[2][5];
};


namespace eval {

  struct parameters {
    const float tempo = 8;

    // mobility tables
    const float mobility_scaling[5] = { 0.0f, 1.0f, 1.0f, 1.0f, 1.0f };

    // piece attack tables
    const float knight_attks[5] = { 3.0f, 9.0f, 9.45f, 14.4f, 27.3f };
    const float bishop_attks[5] = { 3.0f, 9.0f, 9.45f, 14.4f, 27.3f };
    const float rook_attks[5] = { 1.5f, 4.5f, 4.725f, 7.2f, 13.65f };
    const float queen_attks[5] = { 0.75f, 2.25f, 2.3625f, 3.6f, 6.825f };

    // king harassment tables
    const float knight_king[3] = { 1.0, 2.0, 3.0 };
    const float bishop_king[3] = { 1.0, 2.0, 3.0 };
    const float rook_king[5] = { 1.0, 2.0, 3.0, 3.0, 4.0 };
    const float queen_king[7] = { 1.0, 3.0, 3.0, 4.0, 4.0, 5.0, 6.0 };
    const float attacker_weight[5] = { 0.5, 4.0, 8.0, 16.0, 32.0 };
    const float king_shelter[3] = { -2.0, 0.0, 2.0 };

  };

  float evaluate(const position& p);

}

#endif
