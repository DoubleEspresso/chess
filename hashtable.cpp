#include "hashtable.h"

hash_table ttable;

inline size_t pow2(size_t x) {
  return x <= 2 ? x : pow2(x >> 1) << 1;
}



hash_table::hash_table() : sz_mb(0), cluster_count(0) {
  sz_mb = 128 * 1024; 
  cluster_count = 1024 * sz_mb / sizeof(hash_cluster);
  cluster_count = pow2(cluster_count);
  if (cluster_count < 1024) cluster_count = 1024;
  entries = std::unique_ptr<hash_cluster[]>(new hash_cluster[cluster_count]());
  clear();
  
}


void hash_table::clear() {
  memset(entries.get(), 0, sizeof(hash_cluster)*cluster_count);
}



bool hash_table::fetch(const U64& key, hash_data& e) {
  entry * stored = first_entry(key);
  
  for (unsigned i = 0; i<cluster_size; ++i, ++stored) {
    if ((stored->pkey ^ stored->dkey) == key) {
      e.decode(stored->dkey);      
      return true;
    }
  }
  
  return false;
}


bool hash_table::searching(const U64& key, entry& eo) {

  entry * e = first_entry(key);

  for (unsigned i = 0; i < cluster_size; ++i, ++e) {
    if ((e->pkey ^ e->dkey) == key) {

      if (e->is_searching()) {
	eo = *e;
	return true;
      }
    }
    else if (e->empty()) break;
  }

  
  e->dkey ^= search_bit;
  e->pkey = key ^ e->dkey;
  eo = *e;
  return false;
}

void hash_table::save(const U64& key,
		      const U8& depth,
		      const U8& bound,
		      const Move& m,
		      const int16& score) {
  
  entry * e, *replace;

  e = replace = first_entry(key);

  for (unsigned i = 0; i < cluster_size; ++i, ++e) {
    
    if (e->empty() || ((e->pkey) ^ (e->dkey)) == key ) {
      replace = e;
      break;      
    }

    if (e->depth() > depth) {
      replace = e;
    }    
  }
  
  replace->encode(depth, bound, m, score);
  replace->pkey = key ^ replace->dkey;
}
