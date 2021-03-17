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
#ifndef PAWNS_H
#define PAWNS_H

#include <memory>

#include "position.h"


struct pawn_entry {
  pawn_entry() : key(0ULL), score(0) { }

  U64 key;
  int16 score;

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
  U64 semiopen[2];
  U64 qsidepawns[2];
  U64 ksidepawns[2];
  int16 center_pawn_count;
  bool locked_center;
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
