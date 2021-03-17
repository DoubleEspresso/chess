/*
-----------------------------------------------------------------------------
This source file is part of the Havoc chess engine
Copyright (c) 2020 Minniesoft
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
#include "hashtable.h"
#include <xmmintrin.h>
#include <mmintrin.h>

hash_table ttable;

inline size_t pow2(size_t x) {
  return x <= 2 ? x : pow2(x >> 1) << 1;
}



hash_table::hash_table() : sz_mb(0), cluster_count(0) {
  sz_mb = 3 * 128 * 1024;
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

  { // prefetch.. ?
    char * addr = (char*)stored;
    _mm_prefetch(addr, _MM_HINT_T0);
    _mm_prefetch(addr + 32, _MM_HINT_T0);
  }


  for (unsigned i = 0; i < cluster_size; ++i, ++stored) {
    if ((stored->pkey ^ stored->dkey) == key) {
      e.decode(stored->dkey);
      return true;
    }
  }

  return false;
}

void hash_table::save(const U64& key,
  const U8& depth,
  const U8& bound,
  const U8& age,
  const Move& m,
  const int16& score, const bool& pv_node) {

  entry * e, *replace;

  e = replace = first_entry(key);

  for (unsigned i = 0; i < cluster_size; ++i, ++e) {

    // empty entry or hash collision
    if (e->empty()) {
      replace = e;
      break;
    }

    // collision handling (depth, age and pv node)
    else if (((e->pkey) ^ (e->dkey)) == key) {
      if (age - e->age() > 1) {
        replace = e;
      }
      else if (e->depth() - depth > 0 && pv_node) {
        replace = e;
      }
    }

  }

  replace->encode(depth, bound, age, m, score);
  replace->pkey = key ^ replace->dkey;
}
