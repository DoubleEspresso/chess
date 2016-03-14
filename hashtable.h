#pragma once
#ifndef HEDWIG_HASHTABLE_H
#define HEDWIG_HASHTABLE_H

#include "definitions.h"
#include "utils.h"
#include "globals.h"


// the basic table entry
// notes: 5 * 16 + 16 bits = 96 bits / 8 = 12 bytes
struct TableEntry
{
	U16 pkey; // higher 16 bits of poskey
	U16 dkey; // higher 16 bits of data key
	U16 move;
	int16 value;
	int16 static_value;
	U8 depth;
	U8 bound;
};

// stockfish idea: make clusters of tt-entries that fill the cache-line size
// currently tt-entry is 12 bytes x 5 cluster-size = 60 bytes, we pad this
// with 4 bytes of data to = cache line size of modern intel cpus
const int ClusterSize = 5;

struct HashCluster
{
	TableEntry cluster_entries[ClusterSize];
	char padding[4];
};

// the transposition table class, the hash table consists of a power of 2 of 
// clusters each containing 5 entries.
class HashTable
{
public:
	HashTable();
	//HashTable(int size_mb);
	~HashTable();

	void store(U64 key, U64 data, U16 depth, Bound bound, U16 m, int score, int static_value, int iter);
	TableEntry * first_entry(U64 key);
	bool fetch(U64 key, TableEntry& ein);
	bool init();
	

	//void resize();
	void clear();
	U64 check_elts() { return nb_elts; }

private:
	size_t sz_kb;
	size_t clusterCount;
	size_t nb_elts;
	HashCluster * entry;
};

inline TableEntry * HashTable::first_entry(U64 key)
{
	return &entry[key & (clusterCount - 1)].cluster_entries[0];
}

extern HashTable hashTable;

#endif
