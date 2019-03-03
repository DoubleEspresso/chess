#include <memory>
#include <condition_variable>

#include "position.h"
#include "types.h"
#include "hashtable.h"
#include "utils.h"
#include "evaluate.h"
#include "order.h"

struct search_bounds {
  int16 alpha;
  int16 beta;
  int16 best_score;
  U16 depth;
  Move best_move;  
  volatile bool search_finished;
  
  
  void init() {
    alpha = ninf;
    beta = inf;
    best_score = ninf;
    best_move = {};
    depth = 1;
    search_finished = false;
  }
};


Threadpool search_threads(4);
volatile bool slaves_start;
std::condition_variable cv;
search_bounds sb;
unsigned thread_depth = 3;



void Search::start(position& p, U16 depth) {
  
  std::vector<std::unique_ptr<position>> pv;

  util::clock c;
  c.start();
  slaves_start = false;
  bool parallel = true;
  
  for (unsigned i = 0; i < search_threads.size(); ++i) {
    if (i == 0) { sb.init(); }
    pv.emplace_back(make_unique<position>(p));
    pv[i]->set_id(i);
  }

  // launch master
  search_threads.enqueue(iterative_deepening, *pv[0], depth);

  if (parallel)
  { // launch slaves
    std::unique_lock<std::mutex> lock(mtx);
    while(!slaves_start) cv.wait(lock);    
    for (unsigned i = 1; i<search_threads.size(); ++i) {
      search_threads.enqueue(iterative_deepening, *pv[i], depth);
    }
  }
  
  search_threads.wait_finished();
  c.stop();

  
  if (p.is_master()) {
    readout_pv(*pv[0], Score(0), depth);
  }
  
  
  U64 nodes = 0ULL;
  U64 qnodes = 0ULL;
  for(auto& t : pv) {
    std::cout << "id: " << t->id() << " " << t->nodes() << " " << t->qnodes() << std::endl;
    nodes += t->nodes();
    qnodes += t->qnodes();
  }
  std::cout << "time : " << c.ms() << "ms" << std::endl; 
  std::cout << "nodes: " << nodes << std::endl;
  std::cout << "qnodes: " << qnodes << std::endl;
  std::cout << "knps: " << (nodes / c.ms()) << std::endl;
  std::cout << "bestmove " << (SanSquares[bestmoves[0].f] + SanSquares[bestmoves[0].t]) <<
    " ponder " << (SanSquares[bestmoves[1].f] + SanSquares[bestmoves[1].t]) << std::endl;
}


void Search::search_timer() {
  //std::cout << "timer starts" << std::endl;
  //std::cout << "timer stops" << std::endl;
  return;
}


void Search::iterative_deepening(position& p, U16 depth) {
  int16 alpha = ninf;
  int16 beta = inf;
  int16 delta = 15;
  Score eval = ninf;
  
  const unsigned stack_size = 64 + 4;
  node stack[stack_size];
  std::memset(stack, 0, sizeof(node) * stack_size);
  
  
  for (unsigned id = 1 + p.id(); id <= depth; ++id) {
    
    stack->ply = (stack+1)->ply = 0;
    bool fail_low = false;
    bool fail_high = false;
    
    while (true) {      
      if (id >= 4) {
	if (fail_low || alpha <= ninf) alpha = std::max(int16(eval - delta), int16(ninf));
	if (fail_high || beta >= inf) beta = std::min(int16(eval + delta), int16(inf));	
      }
      fail_low = fail_high = false;
      
      
      eval = search<root>(p, alpha, beta, id, stack + 2);
      
      if (p.is_master()) {
	readout_pv(p, eval, id);
	
	if (id >= thread_depth && !slaves_start) {
	  slaves_start = true;
	  cv.notify_all();	
	}
      }
      
      
      if (eval <= alpha) {
	fail_low = true;
	delta += delta;
      }
      else if (eval >= beta) {
	fail_high = true;
	delta += delta;
      }
      else break;
    }   
	
  }
  
}

template<Nodetype type>
Score Search::search(position& p, int16 alpha, int16 beta, U16 depth, node * stack) {

  if (sb.search_finished) { return Score::draw; }
  
  Score best_score = Score::ninf;
  Move best_move = {};
  size_t deferred = 0;

  Move ttm = {}; ttm.type = Movetype::no_type; // refactor me
  Score ttvalue = Score::ninf;
  
  bool in_check = p.in_check();
  stack->in_check = in_check;
  const bool pv_type = (type == root || type == pv);

  std::vector<Move> quiets;

  stack->ply = (stack-1)->ply + 1;
  U16 root_dist = stack->ply;
  
  
  { // mate distance pruning
    Score mating_score = Score(Score::mate - root_dist);
    beta = std::min(mating_score, Score(beta));
    if (alpha >= mating_score) return mating_score;
    
    Score mated_score = Score(Score::mated + root_dist);
    alpha = std::max(mated_score, Score(alpha));
    if (beta <= mated_score) return mated_score;
  }

  
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
  Score static_eval = (ttvalue != Score::ninf ?
		       ttvalue : Score(std::lround(eval::evaluate(p))));

  // forward pruning
  const bool forward_prune = (!in_check &&
			      !pv_type &&
			      !stack->null_search);

  
  // 1. null move pruning
  if (forward_prune &&
      depth >= 2 &&
      static_eval >= beta) {
    
    int16 R = (depth >= 6 ? depth / 2 : 2);
    int16 ndepth = depth - R;
    
    (stack+1)->null_search = true;
    
    p.do_null_move();
    
    Score null_eval = Score(ndepth <= 1 ?
			    -qsearch<non_pv>(p, -beta, -beta+1, 0, stack+1) :
			    -search<non_pv>(p, -beta, -beta+1, ndepth, stack+1));
    
    p.undo_null_move();
    
    (stack+1)->null_search = false;
    
    if (null_eval >= beta) return null_eval;
  }


  // 2. internal iterative deepening
  if (ttm.type == Movetype::no_type &&
      depth >= (pv_type ? 6 : 4) &&
      (pv_type || static_eval + 50 >= beta)) {
    
    int R = (depth >= 6 ? depth / 2 : 2);
    int iid = depth - R;
    
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

  while (mvs.next_move<search_type::main>(p, move)) {

    if (sb.search_finished) { return Score::draw; }

    // thread update (todo)
    
    // edge case: if we deferred a move causing a beta cut - recheck the hashtable and return early
    if (deferred > 0) {
      hash_data e;
      if (ttable.fetch(p.key(), e)) { 
	if (e.score >= beta && e.depth >= depth && e.bound == bound_low) { 
	  return Score(e.score);
	}
      } 
    }
    
    
    if (move.type == Movetype::no_type || !p.is_legal(move)) {
      continue;
    }    

    
    p.do_move(move);
    
    // continue if another thread is already searching this position    
    entry e;
    {
      if (depth > thread_depth && moves_searched > 0 && ttable.searching(p.key(), e)) {      
	p.adjust_nodes(-1);
	p.undo_move(move);
	stack->deferred_moves[deferred++] = move;
	continue;
      }             
    }

    stack->curr_move = move;

    int16 newdepth = depth + in_check;

    // pvs  
	
    Score score = Score::ninf;
    if (moves_searched < 5) {
      score = Score(newdepth <= 1 ? -qsearch<non_pv>(p, -beta, -alpha, 0, stack+1) :
		    -search<non_pv>(p, -beta, -alpha, newdepth-1, stack+1));
    }
    else {
      
      if (move.type == quiet &&
	  !in_check &&
	  best_score <= alpha &&
	  newdepth > 5) {
	newdepth -= 1;
      }      
      
      score = Score(newdepth <= 1 ? -qsearch<non_pv>(p, -alpha-1, -alpha, 0, stack+1) :
		    -search<non_pv>(p, -alpha-1, -alpha, newdepth - 1, stack+1));
      
      if (score > alpha && score < beta) {
	score = Score(newdepth <= 1 ? -qsearch<non_pv>(p, -beta, -alpha, 0, stack+1) :
		      -search<non_pv>(p, -beta, -alpha, newdepth-1, stack+1));
	
	//if (score > alpha) alpha = score;
      }
      
    }
	
        
    /*
    Score score = Score(newdepth <= 1 ? -qsearch<non_pv>(p, -beta, -alpha, 0, stack+1) :
			-search<non_pv>(p, -beta, -alpha, newdepth-1, stack+1));
    */
    
    ++moves_searched;

    if (move.type == Movetype::quiet) quiets.emplace_back(move);
    
    p.undo_move(move);

    if (e.is_searching()) e.unset_searching(p.key());
	


    if (score > best_score) {
      best_score = score;      
      best_move = move;

      if (score >= alpha) {
	alpha = score;
      }
                  
      if (score >= beta) {
	
	if (best_move.type == Movetype::quiet) {
	  p.stats_update(best_move,
			 (stack-1)->curr_move,
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
	  sb.beta = beta;paw
	  sb.best_score = best_score;
	  sb.depth = depth;
	}
	//if (beta < sb.beta && beta > sb.alpha) sb.beta = beta;	
      }
      */
    }
    
  } // end moves loop
    
    
  // re-try deferred moves (already passed legality check)
  
  for (size_t i = 0; i < deferred; ++i) {
    
    p.do_move(stack->deferred_moves[i]);

    stack->curr_move = stack->deferred_moves[i];
    int16 newdepth = depth + in_check;


    // pvs
	
    Score score = Score::ninf;
    if (moves_searched < 5) {
      score = Score(newdepth <= 1 ? -qsearch<non_pv>(p, -beta, -alpha, 0, stack+1) :
		    -search<non_pv>(p, -beta, -alpha, newdepth-1, stack+1));
    }    
    else {            
      if (move.type == quiet &&
	  !in_check &&
	  best_score <= alpha &&
	  newdepth > 5) {
	newdepth -= 1;
      }
      
      score = Score(newdepth <= 1 ? -qsearch<non_pv>(p, -alpha-1, -alpha, 0, stack+1) :
		    -search<non_pv>(p, -alpha-1, -alpha, newdepth-1, stack+1));
      
      if (score > alpha && score < beta) {
	score = Score(newdepth <= 1 ? -qsearch<non_pv>(p, -beta, -alpha, 0, stack+1) :
		      -search<non_pv>(p, -beta, -alpha, newdepth-1, stack+1));
	
	//if (score > alpha) alpha = score;
      }      
    }    
	
    /*
    Score score = Score(newdepth <= 1 ? -qsearch<non_pv>(p, -beta, -alpha, 0, stack+1) :
			-search<non_pv>(p, -beta, -alpha, newdepth-1, stack+1));
    */

    
    ++moves_searched;
    
    if (stack->deferred_moves[i].type == Movetype::quiet) quiets.emplace_back(stack->deferred_moves[i]);
    
    p.undo_move(stack->deferred_moves[i]);
    
    if (score > best_score) {
      best_score = score;
      best_move = stack->deferred_moves[i];

      if (score >= alpha) {
	alpha = score;
      }
      
      if (score >= beta) {

	if (best_move.type == Movetype::quiet) {
	  p.stats_update(best_move, (stack-1)->curr_move,
			 depth, score, quiets, stack->killers);
	}
	
	break;
      }
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
  }
  

  
  if (moves_searched == 0) {
    return (in_check ? Score(Score::mated + root_dist) : Score::draw);
  }

  
  Bound bound = (best_score >= beta ? bound_low :
		 best_score <= alpha ? bound_high : bound_exact);
  ttable.save(p.key(), depth, U8(bound), best_move, best_score);
  
  
  return best_score;
}


template<Nodetype type>
Score Search::qsearch(position& p, int16 alpha, int16 beta, U16 depth, node * stack) {

  if (sb.search_finished) { return Score::draw; }
  
  Score best_score = Score::ninf;
  Move best_move = {};
  best_move.type = Movetype::no_type;

  Move ttm = {};
  ttm.type = Movetype::no_type;
  Score ttvalue = Score::ninf;

  stack->ply = (stack-1)->ply + 1;
  U16 root_dist = stack->ply;
  
  bool in_check = p.in_check();
  stack->in_check = in_check;

  
  
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
	best_score = (Score)std::lround(eval::evaluate(p));
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


  
  U16 moves_searched = 0;
  move_order mvs(p, ttm, stack->killers);
  Move move;

  while (mvs.next_move<search_type::qsearch>(p, move)) {
    

    if (sb.search_finished) { return Score::draw; }

    
    if (move.type == Movetype::no_type || !p.is_legal(move)) {
      continue;
    }


    // see pruning
    if (move != ttm && //0 &&
	move != stack->killers[0] &&
	move != stack->killers[1] &&
	move != stack->killers[2] &&
	move != stack->killers[3] &&
	//!pv_type &&
	!in_check &&
	best_score < alpha &&
	//moves_searched > 1 &&
	p.see(move) < 0) continue;

    
    p.do_move(move);
    p.adjust_qnodes(1);
    
    Score score = Score(-qsearch<type>(p, -beta, -alpha, (depth - 1 <= 0 ? 0 : depth - 1), stack + 1));

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

  /*
  Bound bound = (best_score >= beta ? bound_low :
		 best_score <= alpha ? bound_high : bound_exact);
  ttable.save(p.key(), depth, U8(bound), best_move, best_score);
  */
  
  return best_score;  
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
      
      res += SanSquares[e.move.f] + SanSquares[e.move.t] + " ";
      p.do_move(e.move);
      moves.push_back(e.move);
      
      if (j <= 1) bestmoves[j] = e.move;
    }
    
    while(!moves.empty()) {
      Move m = moves.back();
      p.undo_move(m);
      moves.pop_back();
    }
        
    printf("info score cp %d depth %d pv ",
           eval,
           depth);
    std::cout << res << std::endl;  
}
