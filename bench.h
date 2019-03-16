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
#include <cassert>

#include "position.h"
#include "types.h"
#include "move.h"
#include "utils.h"
#include "search.h"
#include "evaluate.h"
#include "pbil.h"


struct scores {
  size_t correct;
  size_t total;
  double acc_score;
  double mnps_score;
  std::vector<double> times_ms;
  std::vector<U64> nodes;
  std::vector<U64> qnodes;
};


namespace pbil_score {

  size_t iteration = 0;
  double best_score = std::numeric_limits<double>::max();

};

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
  inline double bench(const int& depth, scores& S, bool silent);
  inline void auto_tune();
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
    mvs.generate<pseudo_legal, pieces>();
    
    U64 sz = 0;
    for (int i = 0; i < mvs.size(); ++i) {

      if (!p.is_legal(mvs[i])) {
        continue;
      }
      ++sz;
    }
    return sz;
  }
  
  U64 cnt = 0;
  Movegen mvs(p);
  mvs.generate<pseudo_legal, pieces>();
    
  for (int i = 0; i < mvs.size(); ++i) {

    if (!p.is_legal(mvs[i])) {
      continue;	
    }

    p.do_move(mvs[i]);
    
    cnt += search(p, depth - 1);

    p.undo_move(mvs[i]);
    
  }

  return cnt;
}


inline double pbil_residual(const std::vector<int>& new_params) {

  ++pbil_score::iteration;

  std::vector<parameter<float>*> engine_params {
      eval::Parameters.pt.get(),
      eval::Parameters.sp.get(),
      eval::Parameters.sn.get(),
      eval::Parameters.sb.get(),
      eval::Parameters.sr.get(),
      eval::Parameters.sq.get(),
      eval::Parameters.sk.get(),
      eval::Parameters.nm.get(),
      eval::Parameters.bm.get(),
      eval::Parameters.rm.get(),
      eval::Parameters.qm.get(),
      eval::Parameters.na.get(),
      eval::Parameters.ba.get(),
      eval::Parameters.ra.get(),
      eval::Parameters.qa.get()
  };


  size_t num_floats = engine_params.size();
  size_t param_floats = new_params.size() / 32;

  assert(num_floats == param_floats);

  for (size_t i = 0, idx = 0; idx < engine_params.size(); ++idx) {

    std::bitset<sizeof(float) * CHAR_BIT> b;
    int bidx = 0;

    std::for_each(new_params.begin() + i, new_params.begin() + i + 32,
      [&](int val) { b[bidx++] = val; ++i; });


    const auto val = b.to_ulong();
    float new_value = 0;
    memcpy(&new_value, &val, sizeof(float));

    engine_params[idx]->set(new_value);
  }

  ttable.clear();
  mtable.clear();
  ptable.clear();
  scores S;
  Perft perft;
  unsigned depth = 8;
  double minimized_score = perft.bench(depth, S, true);


  // log best param set thus far
  if (minimized_score < pbil_score::best_score) {
    pbil_score::best_score = minimized_score;
    std::cout << " ==== new pbil best score ===" << std::endl;
    std::cout << "best: " << minimized_score << " acc: " << S.acc_score << " MNPS: " << S.mnps_score << std::endl;
    for (const auto& p : engine_params) {
      p->print();
    }
    std::cout << std::endl;
    std::cout << std::endl;
  }



  return std::abs(minimized_score);

}


inline void Perft::auto_tune() {
  std::vector<parameter<float>*> engine_params{
    eval::Parameters.pt.get(),
    eval::Parameters.sp.get(),
    eval::Parameters.sn.get(),
    eval::Parameters.sb.get(),
    eval::Parameters.sr.get(),
    eval::Parameters.sq.get(),
    eval::Parameters.sk.get(),
    eval::Parameters.nm.get(),
    eval::Parameters.bm.get(),
    eval::Parameters.rm.get(),
    eval::Parameters.qm.get(),
    eval::Parameters.na.get(),
    eval::Parameters.ba.get(),
    eval::Parameters.ra.get(),
    eval::Parameters.qa.get()
  };
  size_t length = engine_params.size() * 32;
  pbil_score::iteration = 0;
  pbil_score::best_score = std::numeric_limits<double>::max();

  pbil p(300, length, 0.7, 0.15, 0.3, 0.05, 1e-6);

  std::vector<int> i0;
  for (auto& p : engine_params) {
    std::bitset<sizeof(float) * CHAR_BIT> bits = p->get_bits();
    for (size_t i = 0; i < bits.size(); ++i) {
      i0.push_back(bits[i]);
    }
  }

  p.set_initial_guess(i0);
  p.optimize(pbil_residual);
}


inline double Perft::bench(const int& depth, scores& S, bool silent) {

  std::vector<std::string> positions{
    "3r1k2/4npp1/1ppr3p/p6P/P2PPPP1/1NR5/5K2/2R5 w - - bm d4d5",
    "2q1rr1k/3bbnnp/p2p1pp1/2pPp3/PpP1P1P1/1P2BNNP/2BQ1PRK/7R b - - bm f6f5",
    "rnbqkb1r/p3pppp/1p6/2ppP3/3N4/2P5/PPP1QPPP/R1B1KB1R w KQkq - bm e5e6",
    "r1b2rk1/2q1b1pp/p2ppn2/1p6/3QP3/1BN1B3/PPP3PP/R4RK1 w - - bm c3d5",
    "2r3k1/pppR1pp1/4p3/4P1P1/5P2/1P4K1/P1P5/8 w - - bm g5g6",
    "1nk1r1r1/pp2n1pp/4p3/q2pPp1N/b1pP1P2/B1P2R2/2P1B1PP/R2Q2K1 w - - bm h5f6",
    "4b3/p3kp2/6p1/3pP2p/2pP1P2/4K1P1/P3N2P/8 w - - bm f4f5",
    "2kr1bnr/pbpq4/2n1pp2/3p3p/3P1P1B/2N2N1Q/PPP3PP/2KR1B1R w - - bm f4f5",
    "3rr1k1/pp3pp1/1qn2np1/8/3p4/PP1R1P2/2P1NQPP/R1B3K1 b - - bm c6e5",
    "2r1nrk1/p2q1ppp/bp1p4/n1pPp3/P1P1P3/2PBB1N1/4QPPP/R4RK1 w - - bm f2f4",
    "r3r1k1/ppqb1ppp/8/4p1NQ/8/2P5/PP3PPP/R3R1K1 b - - bm d7f5",
    "r2q1rk1/4bppp/p2p4/2pP4/3pP3/3Q4/PP1B1PPP/R3R1K1 w - - bm b2b4",
    "2r3k1/1p2q1pp/2b1pr2/p1pp4/6Q1/1P1PP1R1/P1PN2PP/5RK1 w - - bm g4g7",
    "r1bqkb1r/4npp1/p1p4p/1p1pP1B1/8/1B6/PPPN1PPP/R2Q1RK1 w kq - bm d2e4",
    "r2q1rk1/1ppnbppp/p2p1nb1/3Pp3/2P1P1P1/2N2N1P/PPB1QP2/R1B2RK1 b - - bm h7h5",
    "r1bq1rk1/pp2ppbp/2np2p1/2n5/P3PP2/N1P2N2/1PB3PP/R1B1QRK1 b - - bm c5b3",
    "3rr3/2pq2pk/p2p1pnp/8/2QBPP2/1P6/P5PP/4RRK1 b - - bm e8e4",
    "r4k2/pb2bp1r/1p1qp2p/3pNp2/3P1P2/2N3P1/PPP1Q2P/2KRR3 w - - bm g3g4",
    "3rn2k/ppb2rpp/2ppqp2/5N2/2P1P3/1P5Q/PB3PPP/3RR1K1 w - - bm f5h6",
    "2r2rk1/1bqnbpp1/1p1ppn1p/pP6/N1P1P3/P2B1N1P/1B2QPP1/R2R2K1 b - - bm b7e4",
    "r1bqk2r/pp2bppp/2p5/3pP3/P2Q1P2/2N1B3/1PP3PP/R4RK1 b kq - bm f7f6",
    "r2qnrnk/p2b2b1/1p1p2pp/2pPpp2/1PP1P3/PRNBB3/3QNPPP/5RK1 w - - bm f2f4",
    "1rbq1rk1/p1b1nppp/1p2p3/8/1B1pN3/P2B4/1P3PPP/2RQ1R1K w - - bm e4f6",
    "3r2k1/p2r1p1p/1p2p1p1/q4n2/3P4/PQ5P/1P1RNPP1/3R2K1 b - - bm f5d4",
    "r1b1r1k1/1ppn1p1p/3pnqp1/8/p1P1P3/5P2/PbNQNBPP/1R2RB1K w - - bm b1b2",
    "2r4k/pB4bp/1p4p1/6q1/1P1n4/2N5/P4PPP/2R1Q1K1 b - - bm g5c1",
    "5r1k/6pp/1n2Q3/4p3/8/7P/PP4PK/R1B1q3 b - - bm h7h6",
    "r3k2r/pbn2ppp/8/1P1pP3/P1qP4/5B2/3Q1PPP/R3K2R w KQkq - bm f3e2",
    "3r2k1/ppq2pp1/4p2p/3n3P/3N2P1/2P5/PP2QP2/K2R4 b - - bm d5c3",
    "q3rn1k/2QR4/pp2pp2/8/P1P5/1P4N1/6n1/6K1 w - - bm g3f5",
    "6k1/p3q2p/1nr3pB/8/3Q1P2/6P1/PP5P/3R2K1 b - - bm c6d6",
    "1r4k1/7p/5np1/3p3n/8/2NB4/7P/3N1RK1 w - - bm c3d5",
    "1r2r1k1/p4p1p/6pB/q7/8/3Q2P1/PbP2PKP/1R3R2 w - - bm b1b2",
    "r2q1r1k/pb3p1p/2n1p2Q/5p2/8/3B2N1/PP3PPP/R3R1K1 w - - bm d3f5",
    "8/4p3/p2p4/2pP4/2P1P3/1P4k1/1P1K4/8 w - - bm b3b4",
    "1r1q1rk1/p1p2pbp/2pp1np1/6B1/4P3/2NQ4/PPP2PPP/3R1RK1 w - - bm e6e5",
    "q4rk1/1n1Qbppp/2p5/1p2p3/1P2P3/2P4P/6P1/2B1NRK1 b - - bm a8c8",
    "r2q1r1k/1b1nN2p/pp3pp1/8/Q7/PP5P/1BP2RPN/7K w - - bm a4d7",
    "8/5p2/pk2p3/4P2p/2b1pP1P/P3P2B/8/7K w - - bm h3g4",
    "8/2k5/4p3/1nb2p2/2K5/8/6B1/8 w - - bm c4b5",
    "1B1b4/7K/1p6/1k6/8/8/8/8 w - - bm b8a7",
    "rn1q1rk1/1b2bppp/1pn1p3/p2pP3/3P4/P2BBN1P/1P1N1PP1/R2Q1RK1 b - - bm b7a6",
    "8/p1ppk1p1/2n2p2/8/4B3/2P1KPP1/1P5P/8 w - - bm e4c6",
    "8/3nk3/3pp3/1B6/8/3PPP2/4K3/8 w - - bm b5d7",
    "rn1qkb1r/pp2pppp/5n2/3p1b2/3P4/2N1P3/PP3PPP/R1BQKBNR w KQkq - 0 1 bm d1b3",
    "rn1qkb1r/pp2pppp/5n2/3p1b2/3P4/1QN1P3/PP3PPP/R1B1KBNR b KQkq - 1 1 bm f5c8",
    "r1bqk2r/ppp2ppp/2n5/4P3/2Bp2n1/5N1P/PP1N1PP1/R2Q1RK1 b kq - 1 10 bm g4h6",
    "r1bqrnk1/pp2bp1p/2p2np1/3p2B1/3P4/2NBPN2/PPQ2PPP/1R3RK1 w - - 1 12 bm b2b4",
    "rnbqkb1r/ppp1pppp/5n2/8/3PP3/2N5/PP3PPP/R1BQKBNR b KQkq - 3 5 bm e7e5",
    "rnbq1rk1/pppp1ppp/4pn2/8/1bPP4/P1N5/1PQ1PPPP/R1B1KBNR b KQ - 1 5 bm b4c3",
    "r4rk1/3nppbp/bq1p1np1/2pP4/8/2N2NPP/PP2PPB1/R1BQR1K1 b - - 1 12 bm f8b8",
    "rn1qkb1r/pb1p1ppp/1p2pn2/2p5/2PP4/5NP1/PP2PPBP/RNBQK2R w KQkq c6 1 6 bm d4d5",
    "r1bq1rk1/1pp2pbp/p1np1np1/3Pp3/2P1P3/2N1BP2/PP4PP/R1NQKB1R b KQ - 1 9 bm c6d4",
    "rnbqr1k1/1p3pbp/p2p1np1/2pP4/4P3/2N5/PP1NBPPP/R1BQ1RK1 w - - 1 11 bm a2a4",
    "rnbqkb1r/pppp1ppp/5n2/4p3/4PP2/2N5/PPPP2PP/R1BQKBNR b KQkq f3 1 3 bm d7d5",
    "r1bqk1nr/pppnbppp/3p4/8/2BNP3/8/PPP2PPP/RNBQK2R w KQkq - 2 6 bm c4f7",
    "r2q1rk1/2p1bppp/p2p1n2/1p2P3/4P1b1/1nP1BN2/PP3PPP/RN1QR1K1 w - - 1 12 bm e5f6",
    "r1bqkb1r/2pp1ppp/p1n5/1p2p3/3Pn3/1B3N2/PPP2PPP/RNBQ1RK1 b kq - 2 7 bm d4d5",
    "r1bqkb1r/pp3ppp/2np1n2/4p1B1/3NP3/2N5/PPP2PPP/R2QKB1R w KQkq e6 1 7 bm g5f6",
    "r3kb1r/pp1n1ppp/1q2p3/n2p4/3P1Bb1/2PB1N2/PPQ2PPP/RN2K2R w KQkq - 3 11 bm a2a4",
    "r1bq1rk1/pppnnppp/4p3/3pP3/1b1P4/2NB3N/PPP2PPP/R1BQK2R w KQ - 3 7 bm d3h7",
    "r2qkbnr/ppp1pp1p/3p2p1/3Pn3/4P1b1/2N2N2/PPP2PPP/R1BQKB1R w KQkq - 2 6 bm f3e5",
    "rn2kb1r/pp2pppp/1qP2n2/8/6b1/1Q6/PP1PPPBP/RNB1K1NR b KQkq - 1 6 bm b6b3",
    "2rqk2r/pb1nbp1p/4p1p1/1B1n4/Np1N4/7Q/PP3PPP/R1B1R1K1 w kq - bm e1e6",
    "r1bq1rk1/3nbppp/p2pp3/6PQ/1p1BP2P/2NB4/PPP2P2/2KR3R w - - bm d4g7",
    "2kr4/ppq2pp1/2b1pn2/2P4r/2P5/3BQN1P/P4PP1/R4RK1 b - - bm f6g4",
    "r1bqr1k1/pp1n1ppp/5b2/4N1B1/3p3P/8/PPPQ1PP1/2K1RB1R w - - bm e5f7",
    "3r4/2r5/p3nkp1/1p3p2/1P1pbP2/P2B3R/2PRN1P1/6K1 b - - bm c7c3",
    "3b4/p3P1q1/P1n2pr1/4p3/2B1n1Pk/1P1R4/P1p3KN/1N6 w - - bm d3h3", // mate in 15
    "7r/8/pB1p1R2/4k2q/1p6/1Pr5/P5Q1/6K1 w - - bm b6d4", // mate in 15    
    "3r1r1k/1b4pp/ppn1p3/4Pp1R/Pn5P/3P4/4QP2/1qB1NKR1 w - - bm h5h7", // mate in 18
    "1k2r2r/pbb2p2/2qn2p1/8/PP6/2P2N2/1Q2NPB1/R4RK1 b - - bm c6f3",
    "r6k/6R1/p4p1p/2p2P1P/1pq1PN2/6P1/1PP5/2KR4 w - - bm b2b3",
    "r5k1/1bbq1ppp/p6r/P1Nnp3/1p1B4/5B2/1PPQ2PP/R4R1K b - -  bm h6h2",
    "1k4r1/3r1p1q/1p2pQpP/pPp1P1B1/P1Pp2P1/3P1R2/2n1KR2/8 w - - bm f6f7",
    "1rr3k1/2q1ppb1/p2pbnp1/6Q1/1p1BP1P1/P1N2P2/1PP4R/2K2B1R w - - bm g5h6",
    // positional
    "r3r1k1/1p3nqp/2pp4/p4p2/Pn3P1Q/2N4P/1PPR2P1/3R1BK1 w - - 0 1 bm c3e2",
    "4rrk1/pp1b2pp/5n2/3p1N2/8/2QB1qP1/PP3P1P/4RRK1 w - - 0 1 bm e1e8",
    "r6r/p6p/1pnpkn2/q1p2p1p/2P5/2P1P3/P4PP1/1RBQKB1R w K - 0 1 bm d1c2",
    "rn1qk2r/p2pppbp/1p4p1/8/2Ppb3/5NP1/PP2PPBP/R1BQR1K1 w kq - 0 1 bm c1h6",
    "2q2rk1/1pp2ppp/1p2p1b1/3PP3/1b3PQ1/1PN3N1/1P4PP/5RK1 w - - 0 1 bm g1h1",
    "6k1/3b1p2/3pp1p1/2p1n3/2q1P2P/2PnQP1B/1r1B4/5NRK w - - 0 1 bm g1g3",
    "4r1k1/1prnppb1/p2p1np1/q6p/P2BP3/1RN1QB1P/1PP2PP1/4R1K1 b - - 0 1 bm g8h7",
    "2r2rk1/1pb1qppp/p4n2/3npb2/1PNp4/P2P1NP1/1B1QPPBP/2R2RK1 b - - 0 1 bm b7b5",
    "3r2k1/1b3p2/ppBn1qpp/3p4/1P1N1P2/P3P3/2R2P1P/5QK1 b - - 0 1 bm f6e7",
    "r2q1rk1/2pn1ppp/p2p2b1/1p1P4/1P1N2Pb/2P4P/1P1N1P2/R1BQR1K1 b - - 0 1 bm f8e8",
    "r1b1k2r/2qnbppp/pppp1n2/4p3/P1BPP3/2N2N2/1PP1QPPP/R1BR2K1 w kq - 0 1 bm d4e5",
    "3r2k1/2rbp1bp/pp4p1/3PPp2/2PN1P2/3KB3/6PP/2R4R w - - 0 1 bm e5e6",
    "r3rbk1/1b1n1p2/3P1npp/p3qN2/1ppNP3/P4Q1P/1BB2PP1/3RR1K1 w - - 0 1 bm f5e7",
    "r3r1k1/p5bp/2qP2p1/1p3p2/bPp2B2/2P2PN1/3Q2PP/1B2R2K w - - 0 1 bm g3e4",
    "r3k2r/p2n1pp1/2p1pn1p/1p5P/2PP4/5N2/PP1BRPP1/2K4R w kq - 0 1 bm b2b3",
    "r1bqrbk1/2n4p/3p1pp1/pppP3n/4P3/P1N2NPP/1P1B1PB1/R2QR1K1 w - - 0 1 bm f3h2",
    "6k1/2N4p/p3pr2/4q1bn/3p4/3B1bPQ/PPR2P1P/5RK1 w - - 0 1 bm c7a6",
    "r1bqr1k1/3nbppp/p2p4/3N1PP1/1p1B1P2/1P6/1PP1Q2P/2KR2R1 b - - 0 1 bm e7f8",
    "r1br2k1/p1p1qppp/1pn1p3/4P1N1/2PPN3/8/5PPP/1R1Q1RK1 w - - 0 1 bm d1c2",
    "r1bq1rk1/3nbppp/p2pp3/6PQ/1p1BP2P/2NB4/PPP2P2/2KR3R w - - 0 1 bm d4g7",
    "2r2rk1/8/p3pp1p/1bq3p1/2n1P1P1/3B2N1/P3QRPP/2R3K1 w - - 0 1 bm a2a4",
    "2r1r1k1/1p3pp1/p1n1bbqp/2RNp3/1P2P3/1B3N1P/P4PPK/3RQ3 b - - 0 1 bm e8d8",
    "1r1qk2r/5pbp/p1np2b1/4p3/1pP5/3BB3/PPN2PPP/R2Q1RK1 w kq - 0 1 bm d3g6",
    "r4r1k/4Nppp/2p5/p3P3/np2PP2/1n2K3/NP4PP/3R3R w - - 0 1 bm d1b1",
    "r1bb1rk1/pp3p1p/7q/3N2p1/Q2N2nP/6P1/PP2PPB1/R4RK1 w - - 0 1 bm f1c1",
    "r3qr1k/1p1b3p/p2p1b1P/3Pnp2/3N4/2N1BP2/PPQ4P/R4R1K w - - 0 1 bm a1e1",
    "7k/1r5p/5Np1/2pP4/4PP2/6RP/4q1P1/5RK1 w - - 0 1 bm e4e5",
    "1r4k1/3n1p1p/1qbp2p1/4p1b1/1pP1P3/1P1N1P1P/1BB1Q1P1/3R3K w - - 0 1 bm b2c1",
    "r5k1/1b2bp1p/4pnP1/1p1qN1B1/3p1P2/3B2R1/1r2Q1P1/4R1K1 b - - 0 1 bm h7g6",
    "6rk/p2qPQpp/8/4p3/3p4/6P1/P4P1P/3R2K1 w - - 0 1 bm d1e1",
    "r2q1b1k/p1nb2pp/2n5/1pp1p3/2N5/1QNPB1P1/PP2P1BP/R5K1 w - - 0 1 bm c3b5",
    "r4rk1/ppqn1ppp/2pbpn2/4Nb2/2BP4/2N5/PPPBQPPP/R3R1K1 w - - 0 1 bm c4b3",
    "2k5/p4p2/1n2pPq1/2bb2B1/1pp1N1PP/8/PPQ1BP2/6K1 w - - 0 1 bm e2d3",
    "2kr2n1/p2n1Q2/4p1p1/3p4/1p4P1/3B4/PqP5/1N1KR3 w - - 0 1 bm f7e6",
    "k1qr3r/1pp3pp/p2pp3/6b1/R2NP3/1Q6/PP3PPP/2R3K1 w - - 0 1 bm c1c6",
    "6k1/1p3q2/p1r1pp2/2P3p1/1P1PQ1P1/7R/1P3P2/6K1 w - - 0 1 bm h3h6",
    "r2r2k1/1bqnbppp/p3p3/1p2P3/nP1N1B2/P1N2B2/2P3PP/R3QR1K w - - 0 1 bm f3b7",
    "1r4k1/5pp1/6np/p2N1q1r/8/PP4P1/4QP1P/3RR1K1 w - - 0 1 bm h2h4",
    "r1b2rk1/p2nppbp/3p1np1/qNpN4/2PPP3/4BP2/1P1Q2PP/2KR1B1R b - - 0 1 bm a5d2",
    "2r3k1/p1rq1ppp/1pn1p3/3p4/P2P4/bPB1P1P1/N2Q1P1P/R2R1K2 b - - 0 1 bm c6e7",
    "r1b1k2r/pp1n1ppp/4p3/q1p1P3/1b1P4/P1N2N2/1PPQ2PP/R3KB1R w KQkq - 0 1 bm a1b1",
    "2bq1bk1/6r1/1n1p4/NP1Pp3/4PpPn/8/P3B1P1/R2Q1RK1 b - - 0 1 bm b6d7",
    "2r2rk1/1p1nqppp/5n2/pP1p4/6BQ/P1R1P3/1B3PPP/3R2K1 b - - 0 1 bm c8c3",
    "1Q6/3r3k/p4qp1/8/2Rp1nP1/3P1B2/4P3/5K2 b - - 0 1 bm f6d6",
    "2rqkb1r/3n1p1p/p3p1pn/1p1pP1N1/5P2/2N5/PPP3PP/R1BQ1R1K w kq - 0 1 bm a2a4"
  };


  position p;

  limits lims;
  memset(&lims, 0, sizeof(limits));
  lims.depth = depth;

  S.correct = 0;
  S.total = positions.size();
  size_t counter = 1;
  for (const std::string& pos : positions) {    

    std::cout << "iteration: " << pbil_score::iteration << " pos: " << counter << "/" << positions.size() << "\r" << std::flush;
    std::string tmp = pos.substr(0, pos.find(" bm "));
    std::string bm = pos.substr(pos.find("bm ") + 3);
    std::istringstream fen(tmp);
    ++counter;

    p.setup(fen);

    Search::start(p, lims, silent);

    //std::cout << (p.bestmove == bm ? "correct" : "incorrect") << std::endl;
    
    S.correct += (p.bestmove == bm);
    S.times_ms.push_back(p.elapsed_ms);
    S.nodes.push_back(p.nodes());
    S.qnodes.push_back(p.qnodes());
  }

  // avg nps
  double avg_nodes = 0;
  double avg_time_ms = 0;
  double avg_nps = 0;

  for (auto& n : S.nodes) { avg_nodes += n; }
  for (auto& qn : S.qnodes) { avg_nodes += qn; }
  avg_nodes /= (S.nodes.size());
  for (auto& t : S.times_ms) { avg_time_ms += t; }
  avg_time_ms /= S.times_ms.size();
  avg_nps = avg_nodes / avg_time_ms * 1000.0f;

  // convert to Mega nodes / sec
  avg_nps /= 1e6;

  // assume max MNPS ~ 500 (perfect score for speed = 500 MNPS)
  S.mnps_score = avg_nps / 500.0f;
  S.acc_score = ((float)(S.correct / (float)S.total));
  double minimized_score = (1.0f - (0.90 * S.acc_score + 0.10 * S.mnps_score));
  //std::cout << "\n correct: " << S.correct << " total: " << S.total << std::endl;
  //std::cout << " MNPS: " << nps_score << " acc: " << accuracy_score << std::endl;

  return minimized_score;
}
#endif
