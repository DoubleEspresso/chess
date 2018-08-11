#ifndef BITS_H
#define BITS_H

#include <iostream>

#include "types.h"
#include "bitboards.h"

namespace bits {
    
  inline void print(const U64& b) {
    printf("+---+---+---+---+---+---+---+---+\n");
    for (Row r = r8; r >= r1; --r) {
      for (Col c = A; c <= H; ++c) {
        U64 s = bitboards::squares[8*r+c];
        std::cout << (b & s ? "| X " : "|   ");
      }
      std::cout << "|" << std::endl;
      std::cout << "+---+---+---+---+---+---+---+---+" << std::endl;
    }
    std::cout << "  a   b   c   d   e   f   g   h" << std::endl;
  }

  inline int count(const U64& b) { return __builtin_popcountll(b); }  
  
}

#endif
