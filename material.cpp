#include <vector>

#include "material.h"
#include "types.h"
#include "utils.h"
#include "bitboards.h"
#include "squares.h"


material_table mtable;


template<Color c>
int16 evaluate(const position& p, material_entry& e);


inline size_t pow2(size_t x) {
  return x <= 2 ? x : pow2(x >> 1) >> 1;
}


material_table::material_table() : sz_mb(0), count(0) {
  init();
}

void material_table::init() {
  sz_mb = 20 * 1024;
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
    entries[idx].score = evaluate<white>(p, entries[idx]) - evaluate<black>(p, entries[idx]);
  }
}


template<Color c>
int16 evaluate(const position& p, material_entry& e) {
  std::vector<float> material_vals{ 100.0f, 300.0f, 315.0f, 480.0f, 910.0f };
  
}