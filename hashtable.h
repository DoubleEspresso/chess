#pragma once
#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <memory>

#include "types.h"
#include "move.h"

struct entry {
  U8 depth;
  U8 bound;
  int16 value;
  Move move; // 8 bytes
  U16 pkey;
  U16 dkey;
};

const unsigned cluster_size = 4;

struct hash_cluster {
  // based on entry size = 64 bits / 8 = 8 + 8 bytes
  // 16 * 4 = 64 bytes, leaving 0 bytes for cache padding
  entry cluster_entries[cluster_size];
  //char padding[8];
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
	    const U64& dkey,
	    const U8& depth,
	    const U8& bound,
	    const Move& m,
	    const int16& score);
  bool fetch(const U64& key, entry& e);
  bool searching(const U64& key, const U64& dkey);
  void unset_searching(const U64& key);
  inline entry * first_entry(const U64& key);
  void clear();
};

inline entry * hash_table::first_entry(const U64& key) {
  return &entries[key & (cluster_count - 1)].cluster_entries[0];
}

extern hash_table ttable; // global transposition table

#endif
