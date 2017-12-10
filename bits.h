#pragma once

#ifndef SBC_BITS_H
#define SBC_BITS_H

#include "types.h"
#include "definitions.h"
#include "platform.h"

namespace
{
  // print a bitboard to the screen (DBG)
  inline void PrintBits(U64& board)
  {
    printf("   +---+---+---+---+---+---+---+---+\n");
    for (int r = 7; r >= 0; --r)
      {
	for (int c = 0; c < 8; ++c)
	  {
	    int t = 8 * r + c;
	    U64 s = U64(t);
	    if (board & s)  (c == 0 ? printf(" %d | X ", r + 1) : printf("| X "));
	    else (c == 0 ? printf(" %d |   ", r + 1) : printf("|   "));
	  }
	printf("|\n");
	printf("   +---+---+---+---+---+---+---+---+\n");
      }
    printf("     a   b   c   d   e   f   g   h\n\n");
  }

  // SWAR popcount algorithm for U64 datatypes
  inline int count64(U64 b)
  {
    if (!b) return 0;
    b = b - ((b >> 1) & 0x5555555555555555ULL);
    b = (b & 0x3333333333333333ULL) + ((b >> 2) & 0x3333333333333333ULL);
    b = (b + (b >> 4)) & 0x0f0f0f0f0f0f0f0fULL;
    return (int)((b * 0x0101010101010101ULL) >> 56);
  }

  // SWAR popcount algorithm for U32 datatypes
  inline int count32(U64 b)
  {
    if (!b) return 0;
    U32 bl = U32(b ^ (b << 32)); // lower 32 bits
    U32 bu = U32(b ^ (b >> 32)); // upper 32 bits

    int low_count = 0;
    int high_count = 0;

    if (bl)
      {
	bl = bl - ((bl >> 1) & 0x55555555);
	bl = (bl & 0x33333333) + ((bl >> 2) & 0x33333333);
	low_count = (int)((((bl + (bl >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24);
      }
    if (bu)
      {
	bu = bu - ((bu >> 1) & 0x55555555);
	bu = (bu & 0x33333333) + ((bu >> 2) & 0x33333333);
	high_count = (int)((((bu + (bu >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24);
      }
    return (int)(low_count + high_count);
  }

  inline int count(const U64& b)
  {
    return builtin_popcount(b);
  }

  // use when most bits in "b" are 0.
  /*
    int count64_max15(U64 b)
    {
    int count;
    for (count = 0; b; count++)
    b &= b - 1;
    return count;
    }

    int count32_max15(U32 b)
    {
    int count;
    for (count = 0; b; count++)
    b &= b - 1;
    return count;
    }
  */

  const U64 debruijn64 = C64(0x03f79d71b4cb0a89);
  const U32 debruijn32 = C32(0x077CB531U);

  const int idx64[64] =
    {
      0,  1, 48,  2, 57, 49, 28,  3,
      61, 58, 50, 42, 38, 29, 17,  4,
      62, 55, 59, 36, 53, 51, 43, 22,
      45, 39, 33, 30, 24, 18, 12,  5,
      63, 47, 56, 27, 60, 41, 37, 16,
      54, 35, 52, 21, 44, 32, 23, 11,
      46, 26, 40, 15, 34, 20, 31, 10,
      25, 14, 19,  9, 13,  8,  7,  6
    };

  const int idx32[64] =
    {
      0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
      31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9,

      32, 33, 50, 34, 61, 46, 56, 35, 62, 54, 52, 47, 57, 49, 36, 40,
      63, 59, 45, 55, 53, 51, 48, 39, 58, 44, 50, 38, 43, 37, 42, 41
    };

  inline int lsb64(U64 b)
  {
    return idx64[(int)(((b & (~b + 1ULL)) * debruijn64) >> 58)];
  }

  inline int lsb32(U64 b)
  {
    U32 bl = U32(b ^ (b << 32)); // lower 32 bits
    U32 bu = U32(b ^ (b >> 32)); // upper 32 bits

    U32 bn = (bl ? bl : bu);
    int idx = (bl ? 0 : 32);

    return idx32[(int)((((bn & (~bn + 1ULL)) * debruijn32) >> 27) + idx)];
  }

  inline int lsb(U64& b)
  {
#ifdef _MSC_VER
    unsigned long r = 0; builtin_lsb(&r, b);
    return r;
#else
    return lsb64(b);//builtin_lsb(b);
#endif
  };

  inline int pop_lsb(U64& b) {
    const int s = lsb(b); 
    b &= (b - 1);
    return s;
  };
  
  inline bool more_than_one(U64& b) {
    return b & (b - 1);
  };
  
  inline bool empty(const U64& b) {
    return b == 0ULL;
  }
};
#endif
