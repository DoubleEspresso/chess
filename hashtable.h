#pragma once
#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <memory>

#include "types.h"
#include "move.h"


struct entry {
  entry() : pkey(0ULL), dkey(0ULL) { }
  
  U64 pkey;  // zobrist hashing
  U64 dkey;  // 17 bit val, 8 bit depth, 8 bit bound, 8 bit f, 8 bit t, 8 bit type

  
  inline bool empty() { return pkey == 0ULL && dkey == 0ULL; }
  inline bool is_searching() { return (dkey & U64(1ULL << 63)) == U64(1ULL << 63); }
  inline void encode(const U8& depth,
		     const U8& bound,
		     const Move& m,
		     const int16& score) {
    dkey = 0ULL;
    dkey |= U64(m.f); // 8 bits;
    dkey |= (U64(m.t) << 8); // 8 bits
    dkey |= (U64(m.type) << 16); // 12 bits
    dkey |= (U64(bound) << 28); // 8 bits;
    dkey |= (U64(depth+1) << 36); // 8 bits
    dkey |= (U64(score < 0 ? -score : score) << 44); // 16 bits
    dkey |= (U64(score < 0 ? 1ULL : 0ULL) << 60);    
  }
  inline void unset_searching(const U64& key) {
    dkey ^= U64(1ULL << 63);
    pkey = key ^ dkey;
  }

  inline U8 depth() { return U8(dkey & 0xFF000000000); }
};



struct hash_data {
  U8 depth;
  U8 bound;
  int16 score;
  Move move; // 3 bytes
  U16 pkey;
  U16 dkey;

  inline void decode(const U64& dkey) {
    
    U8 f = U8(dkey & 0xFF);
    U8 t = U8((dkey & 0xFF00) >> 8);
    Movetype type = Movetype((dkey & 0x3FF0000) >> 16);
    bound = U8((dkey & 0xFF0000000) >> 28);
    depth = U8((dkey & 0xFF000000000) >> 36);
    score = int16((dkey & 0xFFFF00000000000) >> 44);
    
    int sign = int(dkey & (1ULL << 62));
    score = (sign == 1 ? -score : score);
    move.set(f, t, type);
  }  
};

const unsigned cluster_size = 4;

struct hash_cluster {
  // based on entry size = 64 bits / 8 = 8 + 8 bytes
  // 16 * 4 = 64 bytes, leaving 0 bytes for cache padding
  entry cluster_entries[cluster_size];
};


class hash_table {
 private:
  size_t sz_kb;
  size_t cluster_count;
  std::unique_ptr<hash_cluster[]> entries;

 public:
  hash_table();
  hash_table(const hash_table& o) = delete;
  hash_table(const hash_table&& o) = delete;
  ~hash_table() {}
  
  hash_table& operator=(const hash_table& o) = delete;
  hash_table& operator=(const hash_table&& o) = delete;

  void save(const U64& key,
	    const U8& depth,
	    const U8& bound,
	    const Move& m,
	    const int16& score);
  bool fetch(const U64& key, hash_data& e);
  bool searching(const U64& key, entry& eo);
  inline entry * first_entry(const U64& key);
  void clear();
};

inline entry * hash_table::first_entry(const U64& key) {
  return &entries[key & (cluster_count - 1)].cluster_entries[0];
}

extern hash_table ttable; // global transposition table

#endif
