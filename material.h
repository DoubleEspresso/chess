#pragma once

#ifndef MATERIAL_H
#define MATERIAL_H

#include <memory>

#include "position.h"

struct material_entry {
  U64 key;
  int16 score;
  bool endgame;
  U8 number[5]; // pawn, knight, bishop, rook, queen
};


class material_table {
  private:
    size_t sz_mb;
    size_t count;
    std::unique_ptr<material_entry[]> entries;

    void init();

  public:
    material_table();
    material_table(const material_table& o) = delete;
    material_table(const material_table&& o) = delete;
    material_table& operator=(const material_table& o) = delete;
    material_table& operator=(const material_table&& o) = delete;

    ~material_table() {}

    void clear();
    material_entry * fetch(const position& p);
	  
};


extern material_table mtable;

#endif