#pragma once

#ifndef EVALUATE_H
#define EVALUATE_H

#include "types.h"
#include "utils.h"
#include "pawns.h"
#include "material.h"
#include "parameter.h"
#include "utils.h"

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
  U64 central_pawns[2];
  bool closed_center;
  unsigned kattackers[2][5];
};


namespace eval {

  float evaluate(const position& p);

}

namespace eval {
  extern parameters Parameters;
}

#endif
