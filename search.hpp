/*
-----------------------------------------------------------------------------
This source file is part of the Havoc chess engine
Copyright (c) 2020 Minniesoft
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
#include <memory>
#include <condition_variable>
#include <iostream>
#include <fstream>

#include "position.h"
#include "types.h"
#include "hashtable.h"
#include "utils.h"
#include "evaluate.h"
#include "order.h"
#include "material.h"

struct search_bounds {
  int16 alpha;
  int16 beta;
  int16 best_score;
  U16 depth;
  Move best_move;
  double elapsed_ms;


  void init() {
    elapsed_ms = 0;
    alpha = ninf;
    beta = inf;
    best_score = ninf;
    best_move = {};
    depth = 1;
  }
};

std::ofstream debug_file;
volatile bool slaves_start;
std::condition_variable cv;
search_bounds sb;
unsigned thread_depth = 600;
volatile double elapsed = 0;
unsigned prob_cut_tries = 0;
unsigned prob_cut_successes = 0;

struct move_entry {
  //size_t keyj
  //Move m;
  bool searching;
};

const size_t mv_hash_sz = 1048576; // to start (some power of 2)
move_entry searching_moves[mv_hash_sz];


inline size_t get_idx(position& p, const Move& m) {
  return ((p.key() * (m.f | (m.t << 8) | (m.type << 8))) & (mv_hash_sz - 1));
}

inline bool is_searching(position& p, const Move& m) {
  return searching_moves[get_idx(p, m)].searching;
}

inline void set_searching(position& p, const Move& m) {
  size_t idx = get_idx(p, m);
  searching_moves[idx].searching = true;
  //searching_moves[idx].m = m;
}

inline void unset_searching(position& p, const Move& m) {
  size_t idx = get_idx(p, m);
  searching_moves[idx].searching = false;
  //searching_moves[idx].m = {};
}

inline unsigned reduction(bool pv_node, bool improving, int d, int mc) {
  return bitboards::reductions[(int)pv_node][(int)improving]
    [std::max(0, std::min(d, 64 - 1))][std::max(0, std::min(mc, 64 - 1))];
}

inline float razor_margin(int depth) {
  return 950 * (1 - exp((depth - 64.0) / 20.0));
}


inline float lazy_eval_margin(int depth, bool advanced_pawn) { //, bool pv_node, bool improving) {
  return (advanced_pawn ? -1 : 350 * (1 - exp((depth - 64.0) / 20.0)));
}

void Search::start(position& p, limits& lims, bool silent) {

  std::vector<std::unique_ptr<position>> pv;
  Threadpool search_threads(4);
  Threadpool timer_thread(1);

  slaves_start = false;
  bool parallel = false;
  elapsed = 0;
  UCI_SIGNALS.stop = false;

  { // debug stats
    prob_cut_tries = 0;
    prob_cut_successes = 0;
  }

  if (p.debug_search) {
    debug_file.open("debug.txt");
  }

  for (unsigned i = 0; i < search_threads.size(); ++i) {
    if (i == 0) { sb.init(); }
    pv.emplace_back(util::make_unique<position>(p));
    pv[i]->set_id(i);
  }


  // launch master
  U16 depth = (lims.depth > 0 ? lims.depth : 64); // maxdepth
  searching = true;

  timer_thread.enqueue(search_timer, p, lims);
  search_threads.enqueue(iterative_deepening, *pv[0], depth, silent);

  if (parallel) {
    std::unique_lock<std::mutex> lock(mtx);
    while (!slaves_start && searching) cv.wait(lock);

    for (unsigned i = 1; i < search_threads.size(); ++i) {
      if (searching) search_threads.enqueue(iterative_deepening, *pv[i], depth, silent);
    }
  }

  search_threads.wait_finished();
  UCI_SIGNALS.stop = true;


  U64 nodes = 0ULL;
  U64 qnodes = 0ULL;
  for (auto& t : pv) {
    if (!silent) {
      std::cout << "id: " << t->id() << " " << t->nodes() << " " << t->qnodes() << std::endl;
    }
    nodes += t->nodes();
    qnodes += t->qnodes();
  }

  if (!silent) {
    std::cout << "time : " << elapsed << "ms" << std::endl;
    std::cout << "nodes: " << nodes << std::endl;
    std::cout << "qnodes: " << qnodes << std::endl;
    std::cout << "knps: " << (nodes / (elapsed)) << std::endl;
    std::cout << "probcut: " << prob_cut_successes << " of " << prob_cut_tries << std::endl;
    std::cout << "bestmove " << uci::move_to_string(bestmoves[0]) <<
      " ponder " << uci::move_to_string(bestmoves[1]) << std::endl;
  }

  { // record some stats for benching..
    p.bestmove = uci::move_to_string(bestmoves[0]);
    p.set_nodes_searched(nodes);
    p.set_qnodes_searched(qnodes);
    p.elapsed_ms = elapsed;
  }

  searching = false;

  if (p.debug_search) {
    debug_file.close();
  }

}

void Search::search_timer(position& p, limits& lims) {
  util::clock c;
  c.start();
  bool fixed_time = lims.movetime > 0;
  int delay = 1; // ms
  double time_limit = estimate_max_time(p, lims);
  auto sleep = [delay]() { std::this_thread::sleep_for(std::chrono::milliseconds(delay)); };

  if (fixed_time) {
    do {
      elapsed += c.elapsed_ms();
      sleep();
    } while (!UCI_SIGNALS.stop && searching && elapsed <= lims.movetime);
  }
  else if (time_limit > -1) {
    // dynamic time estimate in a real game
    do {
      elapsed += c.elapsed_ms();
      sleep();
    } while (!UCI_SIGNALS.stop && searching && elapsed <= time_limit);
  }
  else {
    do {
      // analysis mode (infinite time)
      elapsed += c.elapsed_ms();
      sleep();
    } while (!UCI_SIGNALS.stop && searching);
  }
  UCI_SIGNALS.stop = true;
  return;
}

double Search::estimate_max_time(position& p, limits& lims) {
  double time_per_move_ms = 0;
  if (lims.infinite || lims.ponder || lims.depth > 0) return -1;
  else {
    bool sudden_death = lims.movestogo == 0; // no moves until next time control
    bool exact_time = lims.movetime != 0; // searching for an exact number of ms?
    double remainder_ms = (p.to_move() == white ? lims.wtime + lims.winc : lims.btime + lims.binc);
    //material_entry * me = mtable.fetch(p);
    //bool endgame = me->endgame;
    double moves_to_go = 45.0 - 22.5; // (!endgame ? 22.5 : 30.0);

    if (sudden_death && !exact_time) {
      time_per_move_ms = 2.5 * remainder_ms / moves_to_go;
    }
    else if (exact_time) return (double)lims.movetime;
    else if (!sudden_death) {
      time_per_move_ms = remainder_ms / lims.movestogo;
    }
  }
  return time_per_move_ms;
}

void Search::iterative_deepening(position& p, U16 depth, bool silent) {
  int16 alpha = ninf;
  int16 beta = inf;
  int16 delta = 65;
  Score eval = ninf;



  if (p.params.fixed_depth > 0) {
    depth = p.params.fixed_depth;
  }

  const unsigned stack_size = 64 + 4;
  node stack[stack_size];
  std::memset(stack, 0, sizeof(node) * stack_size);

  for (unsigned id = 1 + p.id(); id <= depth; ++id) {
    //for (unsigned id = 1; id <= depth; ++id) {

    if (UCI_SIGNALS.stop) break;

    stack->ply = (stack + 1)->ply = 0;

    while (true) {
      if (id >= 2) {
        alpha = std::max(int16(eval - delta), int16(ninf));
        beta = std::min(int16(eval + delta), int16(inf));
      }

      eval = search<root>(p, alpha, beta, id, stack + 2);

      if (p.is_master() && !UCI_SIGNALS.stop) {
        if (silent) get_bestmove(p);
        else readout_pv(p, eval, id);

        if (id >= thread_depth && !slaves_start) {
          slaves_start = true;
          cv.notify_all();
        }

        if (id == depth) UCI_SIGNALS.stop = true;
      }

      if (UCI_SIGNALS.stop) break;

      if (eval <= alpha) {
        delta += delta;
      }
      else if (eval >= beta) {
        delta += delta;
      }
      else break;
    }
  }

}


template<Nodetype type>
Score Search::search(position& p, int16 alpha, int16 beta, U16 depth, node * stack) {

  if (UCI_SIGNALS.stop) { return Score::draw; }

  assert(alpha < beta);

  Score best_score = Score::ninf;
  Move best_move = {};
  size_t deferred = 0;

  Move ttm = {}; ttm.type = Movetype::no_type; // refactor me
  Score ttvalue = Score::ninf;

  bool in_check = p.in_check();
  stack->in_check = in_check;
  const bool pv_type = (type == root || type == pv);

  std::vector<Move> quiets;

  stack->ply = (stack - 1)->ply + 1;
  U16 root_dist = stack->ply;

  { // mate distance pruning
    Score mating_score = Score(Score::mate - root_dist);
    beta = std::min(mating_score, Score(beta));
    if (alpha >= mating_score) return mating_score;

    Score mated_score = Score(Score::mated + root_dist);
    alpha = std::max(mated_score, Score(alpha));
    if (beta <= mated_score) return mated_score;
  }

  if (p.is_draw()) return Score::draw;

  {  // hashtable lookup
    hash_data e;
    if (ttable.fetch(p.key(), e)) {
      ttm = e.move;
      ttvalue = Score(e.score);

      if (e.depth >= depth) {
        if ((ttvalue >= beta && e.bound == bound_low) ||
          (ttvalue <= alpha && e.bound == bound_high) ||
          (ttvalue > alpha && ttvalue < beta && e.bound == bound_exact))
          return ttvalue;
      }
    }
  }

  // static evaluation
  const bool advanced_pawns = p.pawns_near_promotion(); // either side has pawns on 7th
  const bool stm_pawns_on_7th = p.pawns_on_7th(); // only side to move has pawns on 7th

  Score static_eval = (ttvalue != Score::ninf ?
    ttvalue : !in_check ? Score(std::lround(eval::evaluate(p, lazy_eval_margin(depth, advanced_pawns)))) : Score::ninf);
  stack->static_eval = static_eval;

  if (p.debug_search && ttvalue == Score::ninf && !in_check) {
    debug_file << p.to_fen() << " eval:" << static_eval << "\n";
  }

  // forward pruning
  const bool forward_prune = (!in_check &&
    !pv_type &&
    (stack - 1)->curr_move.type == Movetype::quiet &&
    !stack->null_search &&
    abs(alpha - beta) == 1 && // only prune in null windows (same condition as !pv_node)
    static_eval != ninf);

  // 0. futility pruning
  if (forward_prune && !stm_pawns_on_7th &&
    depth <= 1 &&
    static_eval > mated_max_ply &&
    static_eval + 950 < alpha) return Score(alpha);

  // 0. razoring - prune when losing
  float rm = razor_margin(depth);
  if (
    depth <= 1 &&
    forward_prune &&
    ttm.type == no_type &&
    static_eval + rm <= alpha &&
    //(stack-1)->curr_move.type == capture
    !stm_pawns_on_7th) {

    if (depth <= 1) {
      //prob_cut_tries++;
      Score v = qsearch<non_pv>(p, alpha, beta, 0, stack);
      if (v <= alpha) {
        //prob_cut_successes++;
        return Score(v);
      }
    }
    else {
      //prob_cut_tries++;
      int16 ralpha = alpha - rm;
      Score v = qsearch<non_pv>(p, ralpha, ralpha + 1, 0, stack);
      if (v <= ralpha) {
        //prob_cut_successes++;
        return Score(v);
      }
    }
  }

  // 1. null move pruning - prune when winning
  bool null_move_allowed = (p.to_move() == white ?
    p.non_pawn_material<white>() :
    p.non_pawn_material<black>());
  if (forward_prune &&
    null_move_allowed &&
    depth >= 2 &&
    static_eval >= beta) {

    int16 R = (depth >= 6 ? depth / 2 : 2);
    int16 ndepth = depth - R;

    (stack + 1)->null_search = true;

    p.do_null_move();

    Score null_eval = Score(ndepth <= 1 ?
      -qsearch<non_pv>(p, -beta, -beta + 1, 0, stack + 1) :
      -search<non_pv>(p, -beta, -beta + 1, ndepth, stack + 1));

    p.undo_null_move();

    (stack + 1)->null_search = false;

    if (null_eval >= beta) return Score(beta); // null_eval;

    // threat move - null move failed low (counter array)
    // if from sq of our (quiet) move == to square of threat --> give move ordering bonus to quiet move
    else {
      Move tm = (stack + 1)->curr_move;
      if (tm.type != Movetype::quiet && abs(null_eval - beta) >= 200) {
        stack->threat_move = tm;
      }
    }
  }


  // 2. probcut - prune at larger depths likely uninteresting moves
  if (!pv_type && 0 &&
    !in_check &&
    (stack - 1)->curr_move.type != Movetype::quiet &&
    depth > 9 &&
    static_eval != ninf &&
    !stack->null_search &&
    static_eval < mate_max_ply &&
    static_eval - 950 > beta) {

    prob_cut_tries++;
    Score s = qsearch<non_pv>(p, beta - 1, beta, 0, stack);
    if (s >= beta) {
      prob_cut_successes++;
      return Score(s); // beta);
    }
  }


  // 3. internal iterative deepening - improve move ordering
  if (ttm.type == Movetype::no_type &&
    depth >= (pv_type ? 6 : 4) &&
    (pv_type || static_eval + 50 >= beta)) {

    int16 R = 2 + depth / 6;
    int16 iid = depth - R;

    stack->null_search = true;

    if (pv_type) search<pv>(p, alpha, beta, iid, stack);
    else search<non_pv>(p, alpha, beta, iid, stack);

    stack->null_search = false;

    {
      hash_data e;
      if (ttable.fetch(p.key(), e)) { ttm = e.move; }
    }
  }



  // todo : recheck promotions in move ordering (multiple moves)
  // are being added to the list - but shouldn't be
  /*
  {
    // dbg move ordering
    std::vector<Move> newmvs;
    std::vector<Move> oldmvs;

    move_order o(p, ttm, stack->killers);
    Move move;
    while (o.next_move<search_type::main>(p, move)) {
      if (move.type == Movetype::no_type || !p.is_legal(move)) {
  //std::cout << " .. .. .. skipping illegal mv" << std::endl;
  continue;
      }
      newmvs.push_back(move);
    }


    Movegen mvs(p);
    mvs.generate<pseudo_legal, pieces>();
    for (int j=0; j<mvs.size(); ++j) {
      if (!p.is_legal(mvs[j])) continue;
      oldmvs.push_back(mvs[j]);
    }



    if (newmvs.size() != oldmvs.size()) {
      p.print();
      std::cout << "ERROR: new = " << newmvs.size() << " old = " << oldmvs.size() << std::endl;
      std::cout << "hashmv = " << (SanSquares[ttm.f] + SanSquares[ttm.t]) << std::endl;
      std::cout << "hashmv type = " << (int)(ttm.type) << std::endl;
      std::cout << "hashmv legal = " << p.is_legal_hashmove(ttm) << std::endl;
      std::cout << "hashmv frm p = " << p.piece_on(Square(ttm.f)) << std::endl;
      std::cout << "killer0 = " <<
  (SanSquares[stack->killers[0].f] + SanSquares[stack->killers[0].t]) << std::endl;
      std::cout << "killer0 legal = " << p.is_legal_hashmove(stack->killers[0]) << std::endl;
      std::cout << "killer0 frm p = " << p.piece_on(Square(stack->killers[0].f)) << std::endl;
      std::cout << "killer0 frm type = " << (int)(stack->killers[0].type) << std::endl;
      std::cout << "killer1 = " <<
  (SanSquares[stack->killers[1].f] + SanSquares[stack->killers[1].t]) << std::endl;
      std::cout << "killer1 legal = " << p.is_legal_hashmove(stack->killers[1]) << std::endl;
      std::cout << "killer1 frm p = " << p.piece_on(Square(stack->killers[1].f)) << std::endl;
      std::cout << "killer1 frm type = " << (int)(stack->killers[1].type) << std::endl;

      std::cout << "mate-killer0 = " <<
  (SanSquares[stack->killers[2].f] + SanSquares[stack->killers[2].t]) << std::endl;
      std::cout << "mate-killer0 legal = " << p.is_legal_hashmove(stack->killers[2]) << std::endl;
      std::cout << "mate-killer0 frm p = " << p.piece_on(Square(stack->killers[2].f)) << std::endl;
      std::cout << "mate-killer0 frm type = " << (int)(stack->killers[2].type) << std::endl;
      std::cout << "mate-killer1 = " <<
  (SanSquares[stack->killers[3].f] + SanSquares[stack->killers[3].t]) << std::endl;
      std::cout << "mate-killer1 legal = " << p.is_legal_hashmove(stack->killers[3]) << std::endl;
      std::cout << "mate-killer1 frm p = " << p.piece_on(Square(stack->killers[3].f)) << std::endl;
      std::cout << "mate-killer1 frm type = " << (int)(stack->killers[3].type) << std::endl;

      std::cout << "=== old mvs == = " << std::endl;
      for (auto& om : oldmvs) {
  std::cout << (SanSquares[om.f] + SanSquares[om.t]) << std::endl;
      }

      std::cout << "=== new mvs == = " << std::endl;
      for (auto& n : newmvs) {
  std::cout << (SanSquares[n.f] + SanSquares[n.t]) << std::endl;
      }
      std::cout << "" << std::endl;
      std::cout << "" << std::endl;
    }
  }
  */


  // main search
  U16 moves_searched = 0;
  move_order mvs(p, ttm, stack->killers);
  Move move;
  Move pre_move = (stack - 1)->curr_move;
  Move pre_pre_move = (stack - 2)->curr_move;
  bool improving = stack->static_eval - (stack - 2)->static_eval >= 0;

  while (mvs.next_move<search_type::main0>(p, move, pre_move, pre_pre_move, stack->threat_move)) {

    if (UCI_SIGNALS.stop) { return Score::draw; }

    // thread update (todo)

    // edge case: if we deferred a move causing a beta cut - recheck the hashtable and return early
    //if (deferred > 0) {
    //  hash_data e;
    //  if (ttable.fetch(p.key(), e)) {
    //    if (e.score >= beta && e.depth >= depth && e.bound == bound_low) {
    //      return Score(e.score);
    //    }
    //  }
    //}


    if (move.type == Movetype::no_type || !p.is_legal(move)) {
      continue;
    }


    // see pruning
    if (move != ttm &&
      move != stack->killers[0] &&
      move != stack->killers[1] &&
      move != stack->killers[2] &&
      move != stack->killers[3] &&
      !pv_type &&
      !in_check &&
      best_score < alpha &&
      move.type != Movetype::quiet &&
      move.type != p.is_promotion(move.type) &&
      depth <= 1 &&
      moves_searched > 1 &&
      p.see(move) < 0) continue;


    // continue if another thread is already searching this position
    // note: uncomment only if smp search
    if (depth > thread_depth && moves_searched > 0 && is_searching(p, move)) {
      stack->deferred_moves[deferred++] = move;
      continue;
    }
    else set_searching(p, move);

    p.do_move(move);

    stack->curr_move = move;

    bool gives_check = p.in_check();
    int16 extensions = gives_check;// +in_check;

    int16 reductions = 1;

    if (!pv_type &&
      move.type == quiet &&
      move != ttm &&
      move != stack->killers[0] &&
      move != stack->killers[1] &&
      move != stack->killers[2] &&
      move != stack->killers[3] &&
      !in_check &&
      !p.is_promotion(move.type) &&
      //p.piece_on(Square(move.f)) != pawn &&
      p.piece_on(Square(move.f)) != king &&
      best_score + 250 < alpha &&
      best_score > mated_max_ply) { // &&
      //moves_searched > 1
      //) {
      reductions += 0.5 * reduction(pv_type, improving, depth, moves_searched);

      // reduce with history score
      //if (depth > 8) {
      int hscore = (p.to_move() == white ?
        p.history_stats().score<white>(move, (stack - 1)->curr_move, (stack - 2)->curr_move, stack->threat_move) :
        p.history_stats().score<black>(move, (stack - 1)->curr_move, (stack - 2)->curr_move, stack->threat_move));
      if (hscore < 0)
      {
        reductions += 1;
      }
      //}

    }

    int16 newdepth = depth + extensions - reductions;

    // pvs
    Score score = Score::ninf;
    if (moves_searched < 3) {
      score = Score(newdepth <= 1 ? -qsearch<pv>(p, -beta, -alpha, 0, stack + 1) :
        -search<pv>(p, -beta, -alpha, newdepth - 1, stack + 1));
    }
    else {

      // LMR
      int16 LMR = newdepth;
      if (move.type == quiet &&
        move != ttm &&
        move != stack->killers[0] &&
        move != stack->killers[1] &&
        move != stack->killers[2] &&
        move != stack->killers[3] &&
        !p.is_promotion(move.type) &&
        !in_check &&
        !gives_check &&
        //newdepth >= 2 &&
        best_score <= alpha) {
        unsigned R = reduction(pv_type, improving, depth, moves_searched);
        LMR -= R;
      }

      score = Score(LMR <= 1 ? -qsearch<non_pv>(p, -alpha - 1, -alpha, 0, stack + 1) :
        -search<non_pv>(p, -alpha - 1, -alpha, LMR - 1, stack + 1));


      if (score > alpha) {

        score = Score(newdepth <= 1 ? -qsearch<pv>(p, -beta, -alpha, 0, stack + 1) :
          -search<pv>(p, -beta, -alpha, newdepth - 1, stack + 1));
      }

    }

    ++moves_searched;

    if (move.type == Movetype::quiet) quiets.emplace_back(move);

    p.undo_move(move);

    // note: uncomment only if smp search
    unset_searching(p, move);


    if (score > best_score) {
      best_score = score;
      best_move = move;

      if (score > alpha) {
        alpha = score;
      }

      if (score >= beta) {

        deferred = 0; // skip deferred moves
        if (best_move.type == Movetype::quiet) {
          p.stats_update(best_move,
            (stack - 1)->curr_move,
            depth, score, quiets, stack->killers);
        }

        break;
      }

      // thread update
      /*
      else if (depth >= sb.depth) {
  std::unique_lock<std::mutex> lock(mtx);

  if (best_score >= alpha && best_score < beta &&
      alpha >= sb.alpha && alpha < beta &&
      beta <= sb.beta) {
    sb.alpha = alpha;
    sb.beta = beta;
    sb.best_score = best_score;
    sb.depth = depth;
  }
  //if (beta < sb.beta && beta > sb.alpha) sb.beta = beta;
      }
      */
    }

  } // end moves loop


  // re-try deferred moves (already passed legality check)
  //deferred = 0;
  for (size_t i = 0; i < deferred; ++i) { // deferred; ++i) {

    // are we still searching this move?
    auto dmove = stack->deferred_moves[i];

    //if (is_searching(p, dmove)) {
    //  continue;
    //}
    //else set_searching(p, dmove);

    // see pruning
    //if (move != ttm &&
    //  dmove != stack->killers[0] &&
    //  dmove != stack->killers[1] &&
    //  dmove != stack->killers[2] &&
    //  dmove != stack->killers[3] &&
    //  !pv_type &&
    //  !in_check &&
    //  best_score < alpha &&
    //  move.type != Movetype::quiet &&
    //  depth <= 1 &&
    //  moves_searched > 1 &&
    //  p.see(dmove) < 0) continue;


    p.do_move(dmove);

    bool gives_check = p.in_check();
    int16 extensions = gives_check + in_check;
    int16 reductions = 1;

    if (!pv_type &&
      dmove.type == quiet &&
      dmove != ttm &&
      dmove != stack->killers[0] &&
      dmove != stack->killers[1] &&
      dmove != stack->killers[2] &&
      dmove != stack->killers[3] &&
      !in_check &&
      p.piece_on(Square(dmove.f)) != pawn &&
      p.piece_on(Square(dmove.f)) != king &&
      best_score + 250 < alpha &&
      best_score > mated_max_ply &&
      moves_searched > 1) {

      reductions += 1; // (depth > 14 ? 2 : 1);

      // reduce with history score
      if (depth > 8) {
        int hscore = (p.to_move() == white ?
          p.history_stats().score<white>(dmove, (stack - 1)->curr_move, (stack - 2)->curr_move, stack->threat_move) :
          p.history_stats().score<black>(dmove, (stack - 1)->curr_move, (stack - 2)->curr_move, stack->threat_move));
        if (hscore < 0)
        {
          reductions += 1;
        }
      }
    }

    int16 newdepth = depth + extensions - reductions;


    stack->curr_move = dmove;

    // pvs
    Score score = Score::ninf;
    if (moves_searched < 3) {
      score = Score(newdepth <= 1 ? -qsearch<pv>(p, -beta, -alpha, 0, stack + 1) :
        -search<pv>(p, -beta, -alpha, newdepth - 1, stack + 1));
    }
    else {
      int16 LMR = newdepth;
      if (move.type == quiet &&
        !in_check &&
        dmove != ttm &&
        dmove != stack->killers[0] &&
        dmove != stack->killers[1] &&
        dmove != stack->killers[2] &&
        dmove != stack->killers[3] &&
        !gives_check &&
        //newdepth >= 2 &&
        best_score <= alpha) {
        unsigned R = reduction(pv_type, improving, depth, moves_searched);
        LMR -= R;
      }

      score = Score(LMR <= 1 ? -qsearch<non_pv>(p, -alpha - 1, -alpha, 0, stack + 1) :
        -search<non_pv>(p, -alpha - 1, -alpha, LMR - 1, stack + 1));

      if (score > alpha) {
        score = Score(newdepth <= 1 ? -qsearch<pv>(p, -beta, -alpha, 0, stack + 1) :
          -search<pv>(p, -beta, -alpha, newdepth - 1, stack + 1));
      }
    }

    ++moves_searched;

    if (dmove.type == Movetype::quiet) quiets.emplace_back(dmove);

    p.undo_move(dmove);

    //unset_searching(p, dmove);


    if (score > best_score) {
      best_score = score;
      best_move = dmove;

      if (score >= alpha) {
        alpha = score;
      }

      if (score >= beta) {

        if (best_move.type == Movetype::quiet) {
          p.stats_update(best_move, (stack - 1)->curr_move,
            depth, score, quiets, stack->killers);
        }

        break;
      }


      //else if (depth >= sb.depth) {
      //std::unique_lock<std::mutex> lock(mtx);
      //
      //	if (best_score >= alpha && best_score < beta &&
      //	    alpha >= sb.alpha && alpha < beta &&
      //	    beta <= sb.beta) {
      //	  sb.alpha = alpha;
      //	  sb.beta = beta;
      //	  sb.best_score = best_score;
      //	  sb.depth = depth;
      //	}
      //	//if (beta < sb.beta && beta > sb.alpha) sb.beta = beta;	
      //      }
      //      
    }
  }



  if (moves_searched == 0) {
    return (in_check ? Score(Score::mated + root_dist) : Score::draw);
  }


  Bound bound = (best_score >= beta ? bound_low :
    best_score <= alpha ? bound_high : bound_exact);
  ttable.save(p.key(), depth, U8(bound), U8(stack->ply), best_move, best_score, pv_type);

  return best_score;
}

const std::vector<float> material_vals{ 100.0f, 300.0f, 315.0f, 480.0f, 910.0f };

template<Nodetype type>
Score Search::qsearch(position& p, int16 alpha, int16 beta, U16 depth, node * stack) {

  if (UCI_SIGNALS.stop) { return Score::draw; }


  Score best_score = Score::ninf;
  Move best_move = {};
  best_move.type = Movetype::no_type;

  Move ttm = {};
  ttm.type = Movetype::no_type;
  Score ttvalue = Score::ninf;
  bool pv_type = type == pv;

  stack->ply = (stack - 1)->ply + 1;
  U16 root_dist = stack->ply;

  bool in_check = p.in_check();
  stack->in_check = in_check;


  if (p.is_draw()) return Score::draw;

  {  // hashtable lookup
    hash_data e;
    if (ttable.fetch(p.key(), e)) {
      ttm = e.move;
      ttvalue = Score(e.score);

      if (e.depth >= depth) {
        if ((ttvalue >= beta && e.bound == bound_low) ||
          (ttvalue <= alpha && e.bound == bound_high) ||
          (ttvalue > alpha && ttvalue < beta && e.bound == bound_exact))
          return ttvalue;
      }
    }
  }


  // stand pat
  if (!in_check) {

    best_score = (Score)std::lround(eval::evaluate(p, lazy_eval_margin(1, true)));

    if (p.debug_search) {
      debug_file << p.to_fen() << " eval:" << best_score << "\n";
    }

    if (best_score + 975 < alpha) return best_score;
    if (best_score >= beta) return best_score;
    if (alpha < best_score) alpha = best_score;
  }


  /*
  {
    // dbg move ordering
    std::vector<Move> newmvs;
    std::vector<Move> oldmvs;

    move_order o(p, ttm, stack->killers);
    Move move;
    while (o.next_move<search_type::qsearch>(p, move)) {
      if (move.type == Movetype::no_type || !p.is_legal(move)) {
  //std::cout << " .. .. .. skipping illegal mv" << std::endl;
  continue;
      }
      newmvs.push_back(move);
    }


    Movegen mvs(p);
    if (!in_check) mvs.generate<capture, pieces>();
    else mvs.generate<pseudo_legal, pieces>();

    for (int j=0; j<mvs.size(); ++j) {
      if (!p.is_legal(mvs[j])) continue;
      oldmvs.push_back(mvs[j]);
    }


    if (newmvs.size() != oldmvs.size()) {
      p.print();
      std::cout << "ERROR: new = " << newmvs.size() << " old = " << oldmvs.size() << std::endl;
      std::cout << "hashmv = " << (SanSquares[ttm.f] + SanSquares[ttm.t]) << std::endl;
      std::cout << "hashmv type = " << (int)(ttm.type) << std::endl;
      std::cout << "hashmv legal = " << p.is_legal_hashmove(ttm) << std::endl;
      std::cout << "hashmv frm p = " << p.piece_on(Square(ttm.f)) << std::endl;
      std::cout << "killer0 = " <<
  (SanSquares[stack->killers[0].f] + SanSquares[stack->killers[0].t]) << std::endl;
      std::cout << "killer0 legal = " << p.is_legal_hashmove(stack->killers[0]) << std::endl;
      std::cout << "killer0 frm p = " << p.piece_on(Square(stack->killers[0].f)) << std::endl;
      std::cout << "killer0 frm type = " << (int)(stack->killers[0].type) << std::endl;
      std::cout << "killer1 = " <<
  (SanSquares[stack->killers[1].f] + SanSquares[stack->killers[1].t]) << std::endl;
      std::cout << "killer1 legal = " << p.is_legal_hashmove(stack->killers[1]) << std::endl;
      std::cout << "killer1 frm p = " << p.piece_on(Square(stack->killers[1].f)) << std::endl;
      std::cout << "killer1 frm type = " << (int)(stack->killers[1].type) << std::endl;

      std::cout << "=== old mvs == = " << std::endl;
      for (auto& om : oldmvs) {
  std::cout << (SanSquares[om.f] + SanSquares[om.t]) << std::endl;
      }

      std::cout << "=== new mvs == = " << std::endl;
      for (auto& n : newmvs) {
  std::cout << (SanSquares[n.f] + SanSquares[n.t]) << std::endl;
      }
      std::cout << "" << std::endl;
      std::cout << "" << std::endl;
    }
  }
  */


  U16 qsdepth = in_check ? 1 : 0;

  U16 moves_searched = 0;
  move_order mvs(p, ttm, stack->killers);
  Move move;
  Move pre_move = (stack - 1)->curr_move;
  Move pre_pre_move = (stack - 2)->curr_move;

  while (mvs.next_move<search_type::qsearch>(p, move, pre_move, pre_pre_move, stack->threat_move)) {


    if (UCI_SIGNALS.stop) { return Score::draw; }


    if (move.type == Movetype::no_type || !p.is_legal(move)) {
      continue;
    }


    // qsearch futility pruning
    int idx = int(p.piece_on(Square(move.t)));
    float capture_score = (move.type == capture ? material_vals[idx] :
      move.type == ep ? material_vals[0] :
      move.type == capture_promotion_q ? material_vals[idx] + material_vals[queen] :
      move.type == capture_promotion_r ? material_vals[idx] + material_vals[rook] :
      move.type == capture_promotion_b ? material_vals[idx] + material_vals[bishop] :
      move.type == capture_promotion_n ? material_vals[idx] + material_vals[knight] : 0);
    int margin = 250;
    if (!in_check && capture_score > 0 && (best_score + capture_score + margin < alpha))
    {
      //return Score(alpha); 
      continue;
    }


    // see pruning
    if (
      //move != ttm &&
      //move != stack->killers[0] &&
      //move != stack->killers[1] &&
      //move != stack->killers[2] &&
      //move != stack->killers[3] &&
      //!pv_type &&
      !in_check &&
      best_score < alpha &&
      //moves_searched > 1 &&
      p.see(move) < 0) continue;


    p.do_move(move);
    p.adjust_qnodes(1);

    Score score = Score(-qsearch<type>(p, -beta, -alpha, 0, stack + 1));

    ++moves_searched;

    p.undo_move(move);


    if (score > best_score) {
      best_score = score;
      best_move = move;

      if (score >= alpha) alpha = score;
      if (score >= beta) { break; }
    }

  }


  if (moves_searched == 0 && in_check) {
    return Score(Score::mated + root_dist);
  }


  //Bound bound = (best_score >= beta ? bound_low :
  //  best_score <= alpha ? bound_high : bound_exact);
  //ttable.save(p.key(), qsdepth, U8(bound), stack->ply, best_move, best_score, pv_type);
  //

  return best_score;
}


void Search::get_bestmove(position& p) {
  hash_data e;
  ttable.fetch(p.key(), e);

  if (e.move.type != Movetype::no_type &&
    e.move.f != e.move.t &&
    p.is_legal(e.move))
    bestmoves[0] = e.move;
}

void Search::readout_pv(position& p, const Score& eval, const U16& depth) {

  hash_data e;
  std::string res = "";
  std::vector<Move> moves;

  for (unsigned j = 0;
    ttable.fetch(p.key(), e) &&
    e.move.type != Movetype::no_type &&
    e.move.f != e.move.t &&
    p.is_legal(e.move) &&
    j < depth; ++j) {

    res += uci::move_to_string(e.move) + " ";
    p.do_move(e.move);
    moves.push_back(e.move);

    if (j <= 1) bestmoves[j] = e.move;
  }

  while (!moves.empty()) {
    Move m = moves.back();
    p.undo_move(m);
    moves.pop_back();
  }

  printf("info score cp %d depth %d pv ",
    eval,
    depth);
  std::cout << res << std::endl;
}
