#pragma once

#ifndef BENCH_H
#define BENCH_H

#include <string>
#include <iomanip>
#include <iostream>
#include <cstring>
#include <stdio.h>
#include <algorithm>
#include <string>

#include "position.h"
#include "types.h"
#include "move.h"

class Perft {


 public:
  Perft() {};
  Perft(const Perft& p) = delete;
  Perft(const Perft&& p) = delete;
  Perft& operator=(const Perft& p) = delete;
  Perft& operator=(const Perft&& p) = delete;

  inline void go(const int& depth);
  inline U64 search(position& p, const int& depth);
  inline void divide(position& p, int d);
  
};


inline void Perft::go(const int& depth) {
  
  //Clock timer;
  std::string positions[5] =
    {
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
      "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
      "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
      "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
      "rnbqkb1r/pp1p1ppp/2p5/4P3/2B5/8/PPP1NnPP/RNBQK2R w KQkq - 0 6"
    };
  
  long int results[5][7] = {
			    { 20,  400,  8902,  197281,   4865609, 119060324, 3195901860 },
			    { 48, 2039, 97862, 4085603, 193690690,         0,          0 },
			    { 14,  191,  2812,   43238,    674624,  11030083,  178633661 },
			    { 6,   264,  9467,  422333,  15833292, 706045033,          0 },
			    { 42, 1352, 53392,       0,         0,         0,          0 } };
  
  if (depth > 7) {
    std::cout << "Abort, max depth = 7" << std::endl;
    return;
  }

  U64 nb = 0ULL;
  for (int i = 0; i < 5; ++i) {
    std::istringstream fen(positions[i]);
    position board(fen);
    
    std::cout << "Position : " << positions[i] << std::endl;
    std::cout << "" << std::endl;
    for (int d = 0; d < depth; d++) {
      //timer.start();
      nb = search(board, d + 1);
      //timer.stop();
      std::cout << "depth "
		<< (d + 1) << "\t"
		<< std::right << std::setw(14)
		<< results[i][d]
		<< "\t" << "perft " << std::setw(14)
		<< nb << "\t " << std::setw(15) << std::endl;
	//<< timer.ms() << " ms " << std::endl;
    }
    std::cout << "" << std::endl;
    std::cout << "" << std::endl;
  }    
}

void Perft::divide(position& p, int d) {
  U64 total = 0;
  Movegen mvs(p);
  
  mvs.generate<pseudo_legal_all, pieces>();
  std::cout << "mvs.size() = " << mvs.size() << std::endl;
  
  for (int i = 0; i < mvs.size(); ++i) {
    if (!p.is_legal(mvs.move(i))) continue;
    p.do_move(mvs.move(i));
    int n = d > 1 ? search(p, d - 1) : 1;
    total += n;
    p.undo_move(mvs.move(i));
    std::cout << SanSquares[mvs[i].from()]
	      << SanSquares[mvs[i].to()]
	      << '\t' << n << std::endl;
  }
  std::cout << "---------------------------------" << std::endl;
  std::cout << "total " << '\t' << total << std::endl;
}

inline U64 Perft::search(position& p, const int& depth) {
  if (depth == 1) {
    Movegen mvs(p);
    mvs.generate<pseudo_legal_all, pieces>();
    U64 sz = 0;
    for (int i = 0; i < mvs.size(); ++i) {
      if (p.is_legal(mvs.move(i))) ++sz;
    }
    return sz;
  }
  
  U64 cnt = 0;

  Movegen mvs(p);
  mvs.generate<pseudo_legal_all, pieces>();
  
  for (int i = 0; i < mvs.size(); ++i) {

    if (!p.is_legal(mvs.move(i))) continue;
    
    p.do_move(mvs[i]);
    
    cnt += search(p, depth - 1);
    
    p.undo_move(mvs[i]);
  }

  return cnt;
}

#endif
