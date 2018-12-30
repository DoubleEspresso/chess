#include "hashtable.h"

hash_table ttable;

inline size_t pow2(size_t x) {
  return x <= 2 ? x : pow2(x >> 1) << 1;
}



hash_table::hash_table() : sz_kb(0), cluster_count(0) {
  
  sz_kb = 128 * 1024; // 128 mb * 1024 kb / 1 mb
  cluster_count = 1024 * sz_kb / sizeof(hash_cluster);
  cluster_count = pow2(cluster_count);
  if (cluster_count < 1024) cluster_count = 1024;
  entries = std::unique_ptr<hash_cluster[]>(new hash_cluster[cluster_count]());
  clear();
  
}

void hash_table::clear() {
  memset(entries.get(), 0, sizeof(hash_cluster)*cluster_count);
}
