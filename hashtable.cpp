#include <string.h>

#include "hashtable.h"
#include "opts.h"

HashTable hashTable;

HashTable::HashTable() : entry(0) { }

HashTable::~HashTable() {
  if (entry) {
    printf("..deleting TableEntry pointer\n");
    delete[] entry; entry = 0;
  }
}
void HashTable::clear() {
  memset(entry, 0, sizeof(HashCluster) * clusterCount);
}

bool HashTable::init()
{
  clusterCount = 0;
  sz_kb = options["hash table size mb"] * 1024;
  if (sz_kb > 0) clusterCount = 1024 * sz_kb / sizeof(HashCluster);

  clusterCount = nearest_power_of_2(clusterCount);
  clusterCount = clusterCount <= 256 ? 256 : clusterCount;

  if (!entry && (entry = new HashCluster[clusterCount]())) {
    //printf("..initialized main hash table: size %3.1f kb, %lu elts.\n", float(clusterCount)*float(sizeof(HashCluster)) / float(1024.0), clusterCount);
  }
  else {
    printf("..[HashTable] alloc failed\n");
    return false;
  }
  clear();
  return true;
}

bool HashTable::fetch(U64 key, TableEntry& ein) {
  TableEntry * e = first_entry(key);
  U16 key16 = key >> 48;

  for (unsigned i = 0; i < ClusterSize; ++i, ++e) {
    if ((e->pkey > 0) && (((e->pkey) ^ (e->dkey)) == key16)) {
      ein = *e;
      return true;
    }
  }
  return false;
}

void HashTable::store(U64 key, U64 data, U8 depth, Bound bound, U16 m, int score, int static_value, bool pv_node) {
  TableEntry * e, *replace;
  U16 key16 = key >> 48;
  U16 data16 = data >> 48;

  e = replace = first_entry(key);

  for (unsigned i = 0; i < ClusterSize; ++i, ++e) {
    // empty entry or found collision
    if ((!e->pkey) || (((e->pkey) ^ (e->dkey)) == key16)) {
      if (!m)
	m = e->move;

      // update replace pointer, and break
      replace = e;
      break;
    }

    // replacement strategy for each cluster -- needs work
    if (e->depth > depth || 
        (e->bound != BOUND_EXACT && bound == BOUND_EXACT))
      replace = e;
  }

  replace->pkey = key16^data16;
  replace->dkey = data16;
  replace->depth = depth + 1; // to adjust negative qsearch depths
  replace->bound = bound;
  replace->move = m;
  replace->value = (int16)score;
  replace->static_value = (int16)static_value;
  replace->pvNode = pv_node;

}
