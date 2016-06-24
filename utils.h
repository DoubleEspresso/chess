#pragma once

#ifndef HEDWIG_UTILS_H
#define HEDWIG_UTILS_H

#include <time.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <iomanip>

#include "types.h"
#include "definitions.h"
#include "globals.h"

#ifdef __linux
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
class lClock
{
 public:
  void start() { gettimeofday(&strt, NULL); }
  void stop() { gettimeofday(&ed, NULL); }
  double seconds() { return (double)(ed.tv_sec - strt.tv_sec); }
  double useconds() { return (double)(ed.tv_usec - strt.tv_usec); }
  double ms() { return ((seconds()) * 1000 + useconds() / 1000); }
  double elapsed_sec() { stop(); double secs = seconds(); start(); return secs; }
  double elapsed_ms() { stop(); double msecs = ms(); start(); return msecs; }

 private:
  struct timeval strt, ed;
};
#endif

class Clock
{
 public:
  Clock() {};
  ~Clock() {};

  void start() { start_time = clock(); }
  void stop() { stop_time = clock(); }

  double seconds() { return (stop_time - start_time) / double(CLOCKS_PER_SEC); }
  double ms() { return seconds() * double(1000); }
  double elapsed_sec() { stop(); double secs = seconds(); start(); return secs; }
  double elapsed_ms() { stop(); double msecs = ms(); start(); return msecs; }

  void print(const char *desc)
  {
    stop();
    double secs = seconds();
    double ms = (secs > 0) ? (1000 * secs) : 0;

    printf("%s\t%7.3f(s)\t%7.0f(ms)\n",
	   desc, secs, ms);
  }
 private:
  clock_t start_time;
  clock_t stop_time;
  U64 start_ticks;
  U64 stop_ticks;
};


// return the color of the input piece
inline Color color_of(int p)
{
  return (p <= W_KING ? WHITE : BLACK);
};

// given a square, return the row
inline int ROW(int r)
{
  return (r >> 3);
};

// given a square, return the column
inline int COL(int c)
{
  return c & 7;
};

// return row dist between two squares
inline int row_distance(int s1, int s2)
{
  return ((ROW(s1)) - ROW((s2)) > 0 ? (ROW(s1)) - (ROW(s2)) : (ROW(s2)) - (ROW(s1)));
};

// return the col distance between two squares
inline int col_distance(int s1, int s2)
{
  return ((COL(s1)) - (COL(s2)) > 0 ? (COL(s1)) - (COL(s2)) : (COL(s2)) - (COL(s1)));
};

// gets the largest distance (row or col distance)
inline int max_distance(int s1, int s2)
{
  int rd = row_distance(s1, s2);
  int cd = col_distance(s1, s2);
  return (rd > cd ? rd : cd);
};

// true if 2 squares are on a col, row, or diagonal
inline bool aligned(int s1, int s2)
{
  return (row_distance(s1, s2) == 0) || (col_distance(s1, s2) == 0) ||
    row_distance(s1, s2) == col_distance(s1, s2);
};

// test if two squares on the same diagonal
inline bool on_diagonal(int s1, int s2)
{
  return row_distance(s1, s2) == col_distance(s1, s2);
};

// true if 3 squares are on a col, row, or diagonal
inline bool aligned(int s1, int s2, int s3)
{
  return (col_distance(s1, s2) == 0 && col_distance(s1, s3) == 0) ||
    (row_distance(s1, s2) == 0 && row_distance(s1, s3) == 0) ||
    (on_diagonal(s1, s3) && on_diagonal(s1, s2) && on_diagonal(s2, s3));
};

// test if square is on the board
inline bool on_board(int s)
{
  return s >= A1 && s <= H8;
};

inline bool same_row(int s1, int s2)
{
  return ROW(s1) == ROW(s2);
}

inline bool same_col(int s1, int s2)
{
  return COL(s1) == COL(s2);
}

// given a move, return the start square of the piece
inline int from_sq(U16 m)
{
  return int(m & 0x3f);
};

// given a move, return the end square of the piece
inline int to_sq(U16 m)
{
  return int((m & 0xfc0) >> 6);
};

// given a move, its type (capture, promotion etc.)
inline int movetype(U16 m)
{
  return int((m & 0xf000) >> 12);
};

// returns true if more than one bit in input
// bitmap is set
inline bool more_than_one(U64& b)
{
  return b & (b - 1);
};

// return a bitboard of squares behind
// a pawn (depends on color)
inline U64 behind_square(U64& col, int color, int square)
{
  return (color == WHITE ? col >> 8 * ROW(square) : col << 8 * ROW(square));
};


inline U64 infrontof_square(U64& col, int color, int square)
{
  return (color == WHITE ? col << 8 * (ROW(square)+1) : col >> 8* (8-ROW(square)) );
};

// get the from square from an encoded move
inline int get_from(U16 m)
{
  return int(m & 0x3f);
};

// get the to square from an encoded move
inline int get_to(U16 m)
{
  return int((m & 0xfc0) >> 6);
};

// get the move type from an encoded move
inline int get_movetype(U16 m)
{
  return int((m & 0xf000) >> 12);
};

// for hash table elements/size computation (most-sig-bit would be faster).
inline size_t nearest_power_of_2(size_t x)
{
  return x <= 2 ? x : nearest_power_of_2(x >> 1) << 1;
}

#endif
