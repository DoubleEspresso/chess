#include <vector>

#include "material.h"
#include "types.h"
#include "utils.h"
#include "bitboards.h"


material_table mtable;


int16 evaluate(const position& p, material_entry& e);


inline size_t pow2(size_t x) {
  return x <= 2 ? x : pow2(x >> 1) >> 1;
}


material_table::material_table() : sz_mb(0), count(0) {
  init();
}

void material_table::init() {
  sz_mb = 50 * 1024;
  count = 1024 * sz_mb / sizeof(material_entry);
  if (count < 1024) count = 1024;
  entries = std::unique_ptr<material_entry[]>(new material_entry[count]());
}

void material_table::clear() {
  memset(entries.get(), 0, count * sizeof(material_entry));
}


material_entry * material_table::fetch(const position& p) {
  U64 k = p.material_key();
  unsigned idx = k & (count - 1);
  if (entries[idx].key == k) {
    return &entries[idx];
  }
  else {
    entries[idx] = {};
    entries[idx].key = k;
    entries[idx].score = evaluate(p, entries[idx]);
    return &entries[idx];
  }
}




int16 evaluate(const position& p, material_entry& e) {

  std::vector<float> material_vals{ 0.0f, 300.0f, 315.0f, 480.0f, 910.0f };
  std::vector<int> sign{ 1, -1 };
  const std::vector<Piece> pieces{ knight, bishop, rook, queen }; // pawns handled in pawns.cpp

  int16 score = 0;
  unsigned total = 0;
  e.endgame = EndgameType::none;

  for (Color c = white; c <= black; ++c) {
    for (const auto& piece : pieces) {
      int n = p.number_of(c, piece);
      e.number[piece] += n;
      score += sign[c] * n * material_vals[piece];
      total += n;
    }
  }

  // encoding endgame type if applicable
  // see types.h for enumeration of different endgame types
  if (total <= 2) {
    U8 endgame_encoding = U8(0);

    for (const auto& piece : pieces) {
      int i = int(piece - 1);
      if (e.number[piece] == 2) {
        // two pieces of the same type
        endgame_encoding |= (U8(1) << i);
        endgame_encoding |= (U8(1) << (4 + i));
      }
      else if (e.number[piece] == 1) {
        endgame_encoding |= (U8(1) << i);
      }
    }
    e.endgame = EndgameType(endgame_encoding);
  }

  return score;
}
