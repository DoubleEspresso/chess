#include <memory>
#include <condition_variable>

#include "position.h"
#include "types.h"
#include "hashtable.h"
#include "utils.h"
#include "evaluate.h"

Threadpool search_threads(4);
volatile bool slaves_start;
std::condition_variable cv;

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
  for(auto& t : pv) {
    std::cout << "id: " << t->id() << " " << t->nodes() << std::endl;
    nodes += t->nodes();
  }
  std::cout << "time : " << c.ms() << "ms" << std::endl; 
  std::cout << "nodes: " << nodes << std::endl;
  std::cout << "knps: " << (nodes / c.ms()) << std::endl;
}


void Search::search_timer() {
  //std::cout << "timer starts" << std::endl;
  //std::cout << "timer stops" << std::endl;
  return;
}


void Search::iterative_deepening(position& p, U16 depth) {
  int16 alpha = ninf;
  int16 beta = inf;
  Score eval = ninf;
  
  const unsigned stack_size = 64 + 4;
  node stack[stack_size];
  std::memset(stack, 0, sizeof(node) * stack_size);
  
  for (unsigned id = 1 + p.id(); id <= depth; ++id) {
    
    stack->ply = (stack+1)->ply = 0;

    eval = search<root>(p, alpha, beta, id, stack + 2);
    
    if (p.is_master()) {
      readout_pv(p, eval, id);

      if (id >= thread_depth && !slaves_start) {
	slaves_start = true;
	cv.notify_all();	
      }
    }    
  }

  if (p.is_master()) { sb.search_finished = true; }
}

template<Nodetype type>
Score Search::search(position& p, int16 alpha, int16 beta, U16 depth, node * stack) {

  if (sb.search_finished) { return Score::draw; }
  
  Score best_score = Score::ninf;
  Move best_move = {};
  size_t deferred = 0;

  Move ttm;
  Score ttvalue = Score::ninf;
  
  bool in_check = p.in_check();
  stack->in_check = in_check;
  const bool pv_type = (type == root || type == pv);
  //const bool forward_prune = (!in_check && !pv_type);

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
	    (ttvalue < alpha && e.bound == bound_high) ||
	    (ttvalue >= alpha && ttvalue < beta && e.bound == bound_exact))
	  return ttvalue;
      }
    }   
  }
  
  // static evaluation
  Score static_eval = (ttvalue != Score::ninf ?
		       ttvalue : Score(eval::evaluate(p)));

  // forward pruning
  const bool forward_prune = (!in_check &&
			      !pv_type &&
			      !stack->null_search);
  
  // 1. null move pruning
  if (forward_prune &&
      depth >= 2 &&
      static_eval >= beta) {
    
    int16 R = (depth >= 8 ? depth / 2 + (depth - 8)/4 : 2);
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
  

  // main search
  Movegen mvs(p);
  mvs.generate<pseudo_legal, pieces>();
  U16 moves_searched = 0;
  
  for (int i = 0; i < mvs.size(); ++i) {

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
    
    
    if (!p.is_legal(mvs[i])) {
      continue;
    }    
        
    p.do_move(mvs[i]);


    
    // continue if another thread is already searching this position

    
    entry e;
    {
      if (depth > thread_depth && moves_searched > 0 && ttable.searching(p.key(), e)) {      
	p.adjust_nodes(-1);
	p.undo_move(mvs[i]);
	stack->deferred_moves[deferred++] = mvs[i];
	continue;
      }             
    }
    
    
    Score score = Score(depth <= 1 ? -qsearch<non_pv>(p, -beta, -alpha, 0, stack+1) :
			-search<non_pv>(p, -beta, -alpha, depth-1, stack+1));
    
    //std::unique_lock<std::mutex> lock(mtx);        
    ++moves_searched;
    
    p.undo_move(mvs[i]);

    if (e.is_searching()) e.unset_searching(p.key());
	


    if (score > best_score) {
      best_score = score;      
      best_move = mvs[i];

      if (score >= alpha) {
	alpha = score;
      }
                  
      if (score >= beta) {
	// history updates
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
  
  for (size_t i = 0; i < deferred; ++i) {


    p.do_move(stack->deferred_moves[i]);

    
    Score score = Score(depth <= 1 ? -qsearch<non_pv>(p, -beta, -alpha, 0, stack+1) :
			-search<non_pv>(p, -beta, -alpha, depth-1, stack+1));

    ++moves_searched;
    
    p.undo_move(stack->deferred_moves[i]);
    
    if (score > best_score) {
      best_score = score;
      best_move = stack->deferred_moves[i];

      if (score >= alpha) {
	alpha = score;
      }
      
      if (score >= beta) {
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
		 best_score < alpha ? bound_high : bound_exact);
  ttable.save(p.key(), depth, U8(bound), best_move, best_score);
    
  return best_score;
}


template<Nodetype type>
Score Search::qsearch(position& p, int16 alpha, int16 beta, U16 depth, node * stack) {

  if (sb.search_finished) { return Score::draw; }
  
  Score best_score = Score::ninf;
  Move best_move = {};

  Move ttm;
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
	    (ttvalue < alpha && e.bound == bound_high) ||
	    (ttvalue >= alpha && ttvalue < beta && e.bound == bound_exact))
	  return ttvalue;
      }
    }    
  }
  
  
  // stand pat
  if (!in_check) {
    best_score = Score(eval::evaluate(p));
  
    if (best_score >= beta) return Score(best_score);
    if (alpha < best_score) alpha = best_score;
  }


  // main search
  U16 moves_searched = 0;
  Movegen mvs(p);
  if (in_check) { mvs.generate<pseudo_legal, pieces>(); }
  else { mvs.generate<capture, pieces>(); }


  for (int i = 0; i < mvs.size(); ++i) {

    if (sb.search_finished) { return Score::draw; }

    
    if (!p.is_legal(mvs[i])) {
      continue;
    }

    p.do_move(mvs[i]);

    Score score = Score(-qsearch<type>(p, -beta, -alpha, (depth - 1 <= 0 ? 0 : depth - 1), stack + 1));

    ++moves_searched;

    p.undo_move(mvs[i]);


    
    if (score > best_score) {
      best_score = score;
      best_move = mvs[i];

      if (score >= alpha) alpha = score;
      if (score >= beta) { break; }      
    }    
    
  }

  
  if (moves_searched == 0 && in_check) {
    return Score(Score::mated + root_dist);
  }


  
  {
    //std::unique_lock<std::mutex> lock(mtx);
    Bound bound = (best_score >= beta ? bound_low :
		   best_score < alpha ? bound_high : bound_exact);
    ttable.save(p.key(), depth, U8(bound), best_move, best_score);
  }
  
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
