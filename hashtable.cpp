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



bool hash_table::fetch(const U64& key, entry& e) {
  entry * stored = first_entry(key);

  U16 key16 = key >> 48;

  for (unsigned i = 0; i<cluster_size; ++i, ++stored) {
    if ((stored->pkey > 0) &&
	(((stored->pkey) ^ (stored->dkey)) == key16)) {
      e = *stored;
      return true;
    }
  }
  return false;
}


bool hash_table::searching(const U64& key, const U64& dkey) {
  entry * e = first_entry(key);
  U16 key16 = key >> 48;
  U16 data16 = dkey >> 48;
  
  for (unsigned i = 0; i < cluster_size; ++i, ++e) {
    if ((e->pkey ^ e->dkey) == key16) {
      if ((e->bound & 128) == 128) return true;

      break;
    }
  }
  
  e->pkey = key16 ^ data16;
  e->dkey = data16;
  e->bound |= 128;    
  return false;
}

void hash_table::unset_searching(const U64& key) {
  entry * e = first_entry(key);
  U16 key16 = key >> 48;

  for (unsigned i = 0; i < cluster_size; ++i, ++e) {
    if ( (!e->pkey) || ((e->pkey ^ e->dkey) == key16)) {
      
      if ((e->bound & 128) == 128) e->bound ^= 128;
      return;
    }
  }
}

void hash_table::save(const U64& key,
		      const U64& dkey,
		      const U8& depth,
		      const U8& bound,
		      const Move& m,
		      const int16& score) {
  
  entry * e, *replace;
  U16 key16 = key >> 48;
  U16 data16 = dkey >> 48;

  e = replace = first_entry(key);

  for (unsigned i = 0; i < cluster_size; ++i, ++e) {
    
    if ( (!e->pkey) || (((e->pkey) ^ (e->dkey)) == key16) ) {

      if (m.type == Movetype::no_type) {
	e->move = m;
      }

      // note: we always replace on collision
      replace = e;
      break;      
    }

    if (e->depth > depth) {
      replace = e;
    }    
  }

  replace->pkey = key16 ^ data16;
  replace->dkey = data16;
  replace->depth = depth + 1; // to adjust negative qsearch depth
  replace->bound = bound;
  replace->move = m;
  replace->value = score;
}
