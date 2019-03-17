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
#include "epd.hpp"
#include "options.h"



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
  std::unique_ptr<epd> E;
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


inline void update_options_file() {
  using namespace eval;
  opts->set<float>("tempo", Parameters.tempo);
  opts->set<float>("pawn ss", Parameters.sq_score_scaling[pawn]);
  opts->set<float>("knight ss", Parameters.sq_score_scaling[knight]);
  opts->set<float>("bishop ss", Parameters.sq_score_scaling[bishop]);
  opts->set<float>("rook ss", Parameters.sq_score_scaling[rook]);
  opts->set<float>("queen ss", Parameters.sq_score_scaling[queen]);
  opts->set<float>("king ss", Parameters.sq_score_scaling[king]);
  opts->set<float>("pawn  ms", Parameters.mobility_scaling[pawn]);
  opts->set<float>("knight ms", Parameters.mobility_scaling[knight]);
  opts->set<float>("bishop ms", Parameters.mobility_scaling[bishop]);
  opts->set<float>("rook ms", Parameters.mobility_scaling[rook]);
  opts->set<float>("queen ms", Parameters.mobility_scaling[queen]);
  opts->set<float>("pawn as", Parameters.attack_scaling[pawn]);
  opts->set<float>("knight as", Parameters.attack_scaling[knight]);
  opts->set<float>("bishop as", Parameters.attack_scaling[bishop]);
  opts->set<float>("rook as", Parameters.attack_scaling[rook]);
  opts->set<float>("queen as", Parameters.attack_scaling[queen]);
  opts->save_param_file();
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

  // todo: something better here .. 
  eval::Parameters.tempo = engine_params[0]->get();
  eval::Parameters.sq_score_scaling[pawn] = engine_params[1]->get();
  eval::Parameters.sq_score_scaling[knight] = engine_params[2]->get();
  eval::Parameters.sq_score_scaling[bishop] = engine_params[3]->get();
  eval::Parameters.sq_score_scaling[rook] = engine_params[4]->get();
  eval::Parameters.sq_score_scaling[queen] = engine_params[5]->get();
  eval::Parameters.sq_score_scaling[king] = engine_params[6]->get();
  eval::Parameters.mobility_scaling[knight] = engine_params[7]->get();
  eval::Parameters.mobility_scaling[bishop] = engine_params[8]->get();
  eval::Parameters.mobility_scaling[rook] = engine_params[9]->get();
  eval::Parameters.mobility_scaling[queen] = engine_params[10]->get();
  eval::Parameters.attack_scaling[knight] = engine_params[11]->get();
  eval::Parameters.attack_scaling[bishop] = engine_params[12]->get();
  eval::Parameters.attack_scaling[rook] = engine_params[13]->get();
  eval::Parameters.attack_scaling[queen] = engine_params[14]->get();

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
    /*
    std::cout << " ==== new pbil best score ===" << std::endl;
    std::cout << "best: " << minimized_score << " acc: " << S.acc_score << " MNPS: " << S.mnps_score << std::endl;
    for (const auto& p : engine_params) {
      p->print();
    }
    std::cout << std::endl;
    std::cout << std::endl;
    */
    update_options_file();
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

  pbil_score::E = util::make_unique<epd>("A:\\code\\chess\\sbchess\\tuning\\epd\\tests.txt");



  size_t length = engine_params.size() * 32;
  pbil_score::iteration = 0;
  pbil_score::best_score = std::numeric_limits<double>::max();

  // was 300
  pbil p(10, length, 0.7, 0.15, 0.3, 0.05, 1e-6);

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

  std::vector<epd_entry> positions = pbil_score::E->get_positions();

  position p;

  limits lims;
  memset(&lims, 0, sizeof(limits));
  lims.depth = depth;

  S.correct = 0;
  S.total = positions.size();
  size_t counter = 1;
  for (const epd_entry& e : positions) {    

    std::cout << "iteration: " << pbil_score::iteration << " pos: " << counter << "/" << positions.size() << " best score: " << pbil_score::best_score << "\r" << std::flush;
    std::istringstream fen(e.pos);
    ++counter;

    p.setup(fen);

    Search::start(p, lims, silent);
    
    S.correct += (p.bestmove == e.bestmove);
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
