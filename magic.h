#pragma once

#ifndef SBC_MAGIC_H
#define SBC_MAGIC_H

#include "types.h"
#include "definitions.h"

struct MagicTable {	
  U64* attacks;
  U64 mask;
  U64 magic;
  U16 offset;
  U8* index_occ;
  U8 shift;
};

extern MagicTable RMagics[SQUARES];
extern MagicTable BMagics[SQUARES];

namespace Magic {
  bool init();
  bool check_magics();
  U64 attack_bm(int p, int s, U64 block);
  U64 next_rand(int bits);

  inline unsigned index(const U64 &magic,
			const U64 &mask,
			const U64 &occ,
			const U8 &shift) { return unsigned(magic * (mask & occ) >> shift); }

  template<Piece p> inline U64 attacks(const U64 &occ, const U8& s);

  template<> inline U64 attacks<ROOK>(const U64 &occ, const U8& s) {
    unsigned int idx = index(RMagics[s].magic, RMagics[s].mask, occ, RMagics[s].shift);
    return RMagics[s].attacks[RMagics[s].index_occ[idx] + RMagics[s].offset];
  }

  template<> inline U64 attacks<BISHOP>(const U64 &occ, const U8& s) {
    unsigned int idx = index(BMagics[s].magic, BMagics[s].mask, occ, BMagics[s].shift);
    return BMagics[s].attacks[BMagics[s].index_occ[idx] + BMagics[s].offset];
  }
}
#endif
