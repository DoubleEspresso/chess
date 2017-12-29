#pragma once
#ifndef SBC_BENCH_H
#define SBC_BENCH_H

#include "definitions.h"

class Board;

class Benchmark
{
 public:
 Benchmark(BenchType bt, int MAX) : max_iter(MAX) { init(bt); }
 Benchmark() : max_iter(0) {};

  void init(BenchType bt);
  void bench_pawns();
  void bench_minors();
  void bench_bitcount(int N);
  void bench_lsb(int N);
  void bench_domove();
  void bench_see();
  void bench(int d);
  long int perft(Board& b, int d);
  void divide(Board& b, int d);

 private:
  std::string pawn_position[10];
  std::string minor_position[10];
  int max_iter;
};

#endif
