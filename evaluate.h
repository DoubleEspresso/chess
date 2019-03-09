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
  U64 empty;
};


namespace eval {

  struct parameters {
    const float tempo = 2;
    const float mobility_scaling[5] = { 0.0f, 1.0f, 1.0f, 1.0f, 1.0f };
  };

  float evaluate(const position& p);

}

#endif
