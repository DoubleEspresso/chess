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
#include <vector>

#include "position.h"
#include "types.h"
#include "move.h"
#include "utils.h"

class Perft {
  std::vector<double> do_mv_times;
  std::vector<double> undo_mv_times;
  std::vector<double> gen_times;
  std::vector<double> legal_times;
  
  util::clock dom_timer;
  util::clock tot_timer;
  util::clock gen_timer;
  util::clock legal_timer;

 public:
  Perft() {};
  Perft(const Perft& p) = delete;
  Perft(const Perft&& p) = delete;
  Perft& operator=(const Perft& p) = delete;
  Perft& operator=(const Perft&& p) = delete;

  inline void go(const int& depth);
  inline U64 search(position& p, const int& depth);
  inline void divide(position& p, int d);
  inline void gen(position& p, U64& times);
  
};


inline void Perft::go(const int& depth) {
  
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
      tot_timer.start();
      nb = search(board, d + 1);
      tot_timer.stop();
      std::cout << "depth "
		<< (d + 1) << "\t"
		<< std::right << std::setw(14)
		<< results[i][d]
		<< "\t" << "perft " << std::setw(14)
		<< nb << "\t " << std::setw(15)
		<< tot_timer.ms() << " ms " << std::endl;
    }
    std::cout << "" << std::endl;
    std::cout << "" << std::endl;
  }    
}

inline void Perft::gen(position& p, U64& times) {

  tot_timer.start();
  int count = 0;
  for (U64 i=0; i < times; ++i) {
    Movegen mvs(p);
    mvs.generate<pseudo_legal, pieces>();
    count = 0;
    for (int j=0; j<mvs.size(); ++j) {
      if (!p.is_legal(mvs[j])) continue;
      
      p.do_move(mvs[j]);
      p.undo_move(mvs[j]);
      
      ++count;
    }
  }
  tot_timer.stop();
  std::cout << "---------------------------------" << std::endl;
  std::cout << count << " legal mvs" << std::endl;
  std::cout << "time: " << dom_timer.ms() << " ms " << std::endl; 
}

void Perft::divide(position& p, int d) {
  tot_timer.start();
  
  U64 total = 0;

  Movegen mvs(p);
  gen_timer.start();
  mvs.generate<pseudo_legal, pieces>();
  gen_timer.stop();
  gen_times.push_back(gen_timer.ms()*1000);
  
  for (int i = 0; i < mvs.size(); ++i) {

    legal_timer.start();
    if (!p.is_legal(mvs[i])) {
      legal_timer.stop();
      legal_times.push_back(legal_timer.ms()*1000);
      continue;
    }
    legal_timer.stop();
    legal_times.push_back(legal_timer.ms()*1000);
	  
    dom_timer.start();
    p.do_move(mvs[i]);
    dom_timer.stop();
    do_mv_times.push_back(1000*dom_timer.ms());

    int n = d > 1 ? search(p, d - 1) : 1;
    total += n;

    dom_timer.start();
    p.undo_move(mvs[i]);
    dom_timer.stop();
    undo_mv_times.push_back(1000*dom_timer.ms());    

    std::cout << SanSquares[mvs[i].f]
	      << SanSquares[mvs[i].t]
	      << '\t' << n << std::endl;
  }
  tot_timer.stop();
  
  double dm_avg = 0;
  for (auto& t: do_mv_times) dm_avg += t;
  dm_avg /= do_mv_times.size();

  double udm_avg = 0;
  for (auto& t: undo_mv_times) udm_avg += t;
  udm_avg /= undo_mv_times.size();

  double gen_avg = 0;
  for (auto& t : gen_times) gen_avg += t;
  gen_avg /= gen_times.size();

  double legal_avg = 0;
  for (auto& t : legal_times) legal_avg += t;
  legal_avg /= legal_times.size();
  
  std::cout << "---------------------------------" << std::endl;
  std::cout << "total " << '\t' << total << std::endl;
  std::cout << "time " << tot_timer.ms() << " ms " << std::endl;
  std::cout << "do-mv time: " << dm_avg << " ns " << std::endl;
  std::cout << "undo-mv time: " << udm_avg << " ns " << std::endl; 
  std::cout << "gen-avg time: " << gen_avg << " ns " << std::endl;
  std::cout << "legal-avg time: " << legal_avg << " ns " << std::endl;
}

inline U64 Perft::search(position& p, const int& depth) {
  if (depth == 1) {

    Movegen mvs(p);
    //gen_timer.start();
    mvs.generate<pseudo_legal, pieces>();
    //gen_timer.stop();
    //gen_times.push_back(gen_timer.ms()*1000);
    
    U64 sz = 0;
    for (int i = 0; i < mvs.size(); ++i) {
      //legal_timer.start();
      if (!p.is_legal(mvs[i])) {
	//legal_timer.stop();
	//legal_times.push_back(legal_timer.ms()*1000);
	continue;	
      }
      //legal_timer.stop();
      //legal_times.push_back(legal_timer.ms()*1000);
      ++sz;
    }
    return sz;
  }
  
  U64 cnt = 0;
  Movegen mvs(p);
  //gen_timer.start();
  mvs.generate<pseudo_legal, pieces>();
  //gen_timer.stop();
  //gen_times.push_back(gen_timer.ms()*1000);
    
  for (int i = 0; i < mvs.size(); ++i) {

    //legal_timer.start();
    if (!p.is_legal(mvs[i])) {
      //legal_timer.stop();
      //legal_times.push_back(legal_timer.ms()*1000);
      continue;	
    }
    //legal_timer.stop();
    //legal_times.push_back(legal_timer.ms()*1000);
    

    //dom_timer.start();
    p.do_move(mvs[i]);
    //dom_timer.stop();
    //do_mv_times.push_back(1000*dom_timer.ms());
    
    cnt += search(p, depth - 1);

    //dom_timer.start();
    p.undo_move(mvs[i]);
    //dom_timer.stop();
    //undo_mv_times.push_back(1000*dom_timer.ms());
    
  }

  return cnt;
}

#endif
