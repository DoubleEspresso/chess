#include "position.h"
#include "move.h"
#include "types.h"
#include "hashtable.h"

Threadpool search_threads(4);

template<Nodetype type>
Score Search::search(position& p, int16 alpha, int16 beta, U16 depth) {

  Score best_score = Score::ninf;
  Move best_move;
  //const bool in_check = p.in_check();
  //const bool pv_type = (type == root || type == pv);
  //const bool forward_prune = (!in_check && !pv_type);

  // hashtable lookup
  const U64 key = p.key();
  const U64 dkey = p.dkey();  
  entry e;
  Move ttm;
  Score ttvalue = Score::ninf;
  
  if (ttable.fetch(key, e)) {
    ttm = e.move;
    ttvalue = Score(e.value);

    if (e.depth >= depth) {
      return ttvalue;
    }
  }
  
  
  // forward pruning stage


  // main search
  Movegen mvs(p);
  mvs.generate<pseudo_legal, pieces>();
  
  for (int i=0; i<mvs.size(); ++i) {
    
    if (!p.is_legal(mvs[i])) {
      continue;
    }


    p.do_move(mvs[i]);
    
    Score score = Score(depth <= 1 ? 0 : -search<non_pv>(p, -beta, -alpha, depth-1));

    p.undo_move(mvs[i]);

    if (score >= best_score) {
      best_score = score;
      best_move = mvs[i];

      if (score >= alpha) alpha = score;
      if (score >= beta) {
	// history updates
	break;
      }
    }    
  }
  
  ttable.save(key, dkey, depth, U8(type), best_move, best_score);

  return best_score;
}
