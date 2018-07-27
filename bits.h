#ifndef BITS_H
#define BITS_H


namespace bits {  
  
  inline void print(const U64& b) {
    printf("   +---+---+---+---+---+---+---+---+\n");
    for (int r = 7; r >= 0; --r) {
      for (int c = 0; c < 8; ++c) {
        int t = 8 * r + c;
        U64 s = (1ULL << t);
        if (b & s)  (c == 0 ? printf(" %d | X ", r + 1) : printf("| X "));
        else (c == 0 ? printf(" %d |   ", r + 1) : printf("|   "));
      }
      printf("|\n");
      printf("   +---+---+---+---+---+---+---+---+\n");
    }
    printf("     a   b   c   d   e   f   g   h\n\n");
  }

  inline int count(const U64& b) { return __builtin_popcountll(b); }  
  
}

#endif
