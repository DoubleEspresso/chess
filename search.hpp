#include <memory>

#include "position.h"
#include "types.h"
#include "hashtable.h"
#include "utils.h"
#include "evaluate.h"

Threadpool search_threads(5);

static const unsigned DepthZero = 64; // for qsearch 

void Search::start(position& p, U16 depth) {
  
  std::vector<std::unique_ptr<position>> pv;
  
  //search_threads.enqueue(search_timer);
  is_searching = true;

  util::clock c;
  c.start();
  
  for (unsigned i = 0; i < search_threads.size(); ++i) {

    //if (i != 0) continue; // just 1 thread for now

    pv.emplace_back(make_unique<position>(p));
    pv[i]->set_id(i);
    search_threads.enqueue(iterative_deepening, *pv[i], depth);
  }

  search_threads.wait_finished();
  c.stop();
  
  U64 nodes = 0ULL;
  for(auto& t : pv) {
    std::cout << t->nodes() << std::endl;
    nodes += t->nodes();
  }
  std::cout << "time : " << c.ms() << "ms" << std::endl; 
  std::cout << "nodes: " << nodes << std::endl;
  is_searching = false;
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
  
  for (unsigned id = 1; id <= depth; ++id) {
    
    stack->ply = (stack+1)->ply = 0; 
    
    eval = search<root>(p, alpha, beta, id, stack + 2);
    
    if (p.is_master()) {
      readout_pv(p, eval, id);
    }    
  }
  
}

template<Nodetype type>
Score Search::search(position& p, int16 alpha, int16 beta, U16 depth, node * stack) {

  Score best_score = Score::ninf;
  Move best_move = {};

  Move ttm;
  Score ttvalue = Score::ninf;
  
  bool in_check = p.in_check();
  stack->in_check = in_check;
  //const bool pv_type = (type == root || type == pv);
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
	return ttvalue;
      }
    }
  }
  
  // forward pruning


  // main search
  Movegen mvs(p);
  mvs.generate<pseudo_legal, pieces>();
  U16 moves_searched = 0;
  
  for (int i = 0; i < mvs.size(); ++i) {
    
    if (!p.is_legal(mvs[i])) {
      continue;
    }    

        
    p.do_move(mvs[i]);

    
    entry e;
    {
      if (depth > 6 && ttable.searching(p.key(), e)) {      
	p.adjust_nodes(-1);
	p.undo_move(mvs[i]);
	continue;
      }             
    }
    
    
    Score score = Score(depth <= 1 ? -qsearch<non_pv>(p, -beta, -alpha, 0, stack+1) :
			-search<non_pv>(p, -beta, -alpha, depth-1, stack+1));

    
    //std::unique_lock<std::mutex> lock(mtx);
    e.unset_searching(p.key());      

    
    ++moves_searched;
    
    p.undo_move(mvs[i]);

    
    if (score > best_score) {
      best_score = score;
      best_move = mvs[i];

      if (score >= alpha) alpha = score;
      if (score >= beta) {
	// history updates
	break;
      }
    } 
    
  }


  
  if (moves_searched == 0) {
    return (in_check ? Score(Score::mated + root_dist) : Score::draw);
  }

 
  //std::unique_lock<std::mutex> lock(mtx);
  ttable.save(p.key(), depth, U8(type), best_move, best_score);
  
  return best_score;
}


template<Nodetype type>
Score Search::qsearch(position& p, int16 alpha, int16 beta, U16 depth, node * stack) {
  
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

  //ttable.save(p.key(), depth, U8(type), best_move, best_score);
  
  return best_score;  
}


void Search::readout_pv(position& p, const Score& eval, const U16& depth) {
  
    hash_data e;
    std::string res = "";
    std::vector<Move> moves;
    
    for (unsigned j = 0;
	 ttable.fetch(p.key(), e) &&
	   //e.move.type != Movetype::no_type &&
	   //e.move.f != e.move.t &&
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
