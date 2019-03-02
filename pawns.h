#ifndef PAWNS_H
#define PAWNS_H

#include <memory>

#include "position.h"


struct pawn_entry {
  pawn_entry() : key(0ULL), val(0) { }

  U64 key;
  int16 val;

  U64 doubled[2];
  U64 isolated[2];
  U64 backward[2];
  U64 passed[2];
  U64 dark[2];
  U64 light[2];
  U64 king[2];
  U64 attacks[2];
  U64 chaintips[2];
  U64 chainbases[2];
  U64 queenside[2];
  U64 kingside[2];  
};



class pawn_table {
 private:
  size_t sz_mb;
  size_t count;
  std::unique_ptr<pawn_entry[]> entries;

  void init();
  
 public:
  pawn_table();
  pawn_table(const pawn_table& o) = delete;
  pawn_table(const pawn_table&& o) = delete;
  pawn_table& operator=(const pawn_table& o) = delete;
  pawn_table& operator=(const pawn_table&& o) = delete;
  ~pawn_table() {}

  void clear();
  pawn_entry * fetch(const position& p);
};


extern pawn_table ptable; // global pawn hash table

#endif
