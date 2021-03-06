#include <cstring>
#ifdef DEBUG
#include <cassert>
#endif

#include "search.h"
#include "material.h"
#include "threads.h"
#include "evaluate.h"
#include "move.h"
#include "moveselect.h"
#include "uci.h"
#include "opts.h"
#include "book.h"
#include "pgnio.h"

// globals
SignalsType UCI_SIGNALS;
std::vector<MoveList> RootMoves;

namespace
{
  int iter_depth = 0;
  U64 last_time_ms = 0; // print pv frequently
  MoveStats statistics;
  int hash_hits = 0;
  bool fixed_depth = false;
  int stop_depth = 10;
  std::ofstream ofile;
  bool debugging = false;
  U64 dbg_ids = 0;
  
  template<NodeType type>
  int search(Board& b, int alpha, int beta, int depth, Node* stack);
  
  template<NodeType type>
  int qsearch(Board& b, int alpha, int beta, int depth, Node* stack, bool inCheck);

  int adjust_score(int bestScore, int ply);
  void update_pv(U16* pv, U16& move, U16* child_pv);
  void pv_from_tt(Board b, int eval, int d, U16 * pv);
  void ReadoutRootMoves(int depth);
}

namespace Search
{
  void run(Board& b, int dpth, bool pondering)
  {
    
    if (debugging) {
      ofile.open("debug_log.txt", std::ios_base::app);
      dbg_ids = 0;
    }
    
    // init search params
    int alpha = NINF;
    int beta = INF;
    int eval = NINF;

    // search nodes init
    Node stack[64 + 4];
    std::memset(stack, 0, (64 + 4) * sizeof(Node));

    // clear search data
    iter_depth = 1;

    
    // start the timer thread now
    timer_thread->searching = true;
    timer_thread->elapsed = 0;
    timer_thread->sleep_condition.signal();
    
    // main entry point for the fail-hard alpha-beta search
    // the main iterative deepening loop
    int delta = 25;
    for (int depth = 1; depth <= dpth; depth += 1) {
      
      RootMoves.clear();
      statistics.clear(); 
      hash_hits = 0;
      
      while (true) {
	
        if (UCI_SIGNALS.stop) break;

	if (depth >= 2) {
	  alpha = MAX(eval - delta, NINF);
	  beta = MIN(eval + delta, INF);
	}	
	
        (stack + 2)->ply = (stack + 1)->ply = (stack)->ply = 0;	
	
        eval = search<ROOT>(b, alpha, beta, depth, stack + 2);
        iter_depth++;

        std::sort(RootMoves.begin(), RootMoves.end(), MLGreater);

        if (timer_thread->elapsed - last_time_ms >= 3000) ReadoutRootMoves(depth);

	
        if (eval <= alpha) {
          alpha -= delta;
          delta += delta / 2;
        }
        else if (eval >= beta) {
          beta += delta;
          delta += delta / 2;
        }
        else break;
      }
      
      
      if (!UCI_SIGNALS.stop) {
        pv_from_tt(b, eval, depth, (stack + 2)->pv);
        last_time_ms = timer_thread->elapsed;        
      }

      if (fixed_depth && depth == stop_depth) {
	break;
      }
	    
    }
  }

  void from_thread(Board& b, int alpha, int beta, int depth, Node& node) {
    search<SPLIT>(b, alpha, beta, depth, NULL);
  }
  
}

namespace {
  int Reduction(bool pv_node, bool improving, int d, int mc) {
    return Globals::SearchReductions[(int)pv_node][(int)improving][MAX(0, MIN(d, MAXDEPTH - 1))][MAX(0, MIN(mc, MAXDEPTH - 1))];
  }
  
  bool RootsContain(MoveList& move) {
    return std::find(RootMoves.begin(), RootMoves.end(), 
                     MoveList(move)) != RootMoves.end();
  }

  
  float RazorMargin(int depth) {
    return 330 + 100 * depth / 10; //log( (float)depth );
  }
  
  
  template<NodeType type>
  int search(Board& b, int alpha, int beta, int depth, Node* stack) {
    //if (b.is_repition_draw()) return DRAW;
    int eval = NINF;
    int aorig = alpha;

    // node type
    //bool split_node = (type == SPLIT);
    //bool root_node = (type == ROOT);
    bool pv_node = (type == ROOT || type == PV);

    U16 quiets[MAXDEPTH];
    for (int j = 0; j < MAXDEPTH; ++j) quiets[j] = MOVE_NONE;
    int moves_searched = 0;
    int quiets_searched = 0;
    U16 bestmove = MOVE_NONE;
    //U16 threat = MOVE_NONE;
    //bool threat_extension = false;
    bool singular_extension = false;
    MoveList root; // storing/sorting rootmoves
    // update stack info
    (stack + 1)->excludedMove = MOVE_NONE;
    U16 excluded_move = MOVE_NONE; //stack->excludedMove;
    stack->ply = (stack - 1)->ply++;
    stack->currmove = 
      stack->bestmove = (stack + 2)->killer[0] = 
      (stack + 2)->killer[1] = (stack + 2)->killer[2] = 
      (stack + 2)->killer[3] = MOVE_NONE;
    stack->givescheck = false;

    // reductions array specific
    bool improving = false;

    // update last move info
    U16 lastmove = (stack - 1)->currmove;

    // init ttable entries
    TableEntry e;
    U64 key = 0ULL;
    U64 data = 0ULL;
    U16 ttm = MOVE_NONE;
    int ttvalue = NINF;
    int mate_dist = iter_depth - depth + 1; //stack->ply;
    int ttstatic_value = NINF;


    // debugging
    if (debugging) {
      stack->ni.id = dbg_ids++;
      stack->ni.alpha = alpha;
      stack->ni.beta = beta;
      stack->ni.depth = depth;
      stack->ni.type = stack->type;
      stack->ni.stm = b.whos_move();
    }
	
    
    // 1. -- mate distance pruning    
    int mate_val = INF - mate_dist;
    beta = MIN(mate_val, beta);
    if (alpha >= mate_val) {
      //printf("!!DBG (search-mate) mate distance pruning :: depth(%d) alpha(%d),mate_val(%d),beta(%d), bm=%s\n", depth, alpha, mate_val, beta, UCI::move_to_string(stack->bestmove).c_str());
      return mate_val;
    }

    int mated_val = NINF + mate_dist;
    alpha = MAX(mated_val, alpha);
    if (beta <= mated_val) {
      //printf("!!DBG (search-mated) mate distance pruning :: depth(%d) alpha(%d),mated_val(%d),beta(%d), bm=%s\n", depth, alpha, mated_val, beta, UCI::move_to_string(stack->bestmove).c_str());
      return mated_val;
    }
    
    // 2. -- ttable lookup 
    data = b.data_key();
    key = b.pos_key();
    if (hashTable.fetch(key, e)) {
	
	ttm = e.move;
	ttstatic_value = e.static_value;
	ttvalue = e.value;
	//bool can_return  = e.pvNode || e.pvNode == pv_node;
	
	// debugging
	if (debugging) {
	  stack->ni.e = &e;
	  stack->ni.hash_hit = true;
	  stack->ni.move = ttm;
	}
	

	if (e.Depth() >= depth) {
	  
	  // note: many entries in the hash-table are from zero-window searches
	  // should we only store data in the hash-table from non-zero window searches?
	  /*
	    b.print();	
	    std::cout << (e.bound == BOUND_EXACT ? "b-exact" : (e.bound == BOUND_LOW ? "b-low" : "b-high") ) << std::endl;
	    std::cout << "(" << alpha << "," << e.value  << "," << beta << ")" << " seval " << e.static_value << std::endl;
	    std::cout << Eval::evaluate(b) << std::endl;	
	    std::cout << std::endl;
	  */


	  if (e.bound == BOUND_EXACT && e.value > alpha && e.value < beta) {
	    stack->currmove = stack->bestmove = e.move;
	    ++hash_hits;
	    if (debugging) { stack->ni.log_search_end(ofile); }
	    return e.value;
	  }
	  else if (e.bound == BOUND_LOW && e.value >= beta) {
	    if (ttm != MOVE_NONE)
	      statistics.update(b, ttm, lastmove, stack, depth, e.value, quiets);
	    ++hash_hits;
	    if (debugging) { stack->ni.log_search_end(ofile); }
	    return beta; 
	  }
	  else if (e.bound == BOUND_HIGH && e.value <= alpha) {
	    ++hash_hits;
	    if (debugging) { stack->ni.log_search_end(ofile); }
	    return alpha;
	  }
	  
      }
    }
    
    // 3. -- static evaluation of position    
    int static_eval = (ttvalue != NINF ? ttvalue : NINF);
    		       
    if (static_eval == NINF &&
	!b.in_check() &&
	get_movetype((stack-1)->currmove) != CAPTURE  &&
	!is_quiet_promotion((stack-1)->currmove) &&
	!is_cap_promotion((stack-1)->currmove)) {
      static_eval = (ttstatic_value != NINF ? ttstatic_value : Eval::evaluate(b));
    }    
    stack->static_eval = static_eval;
    
    bool do_fwd_prune = !b.in_check() &&
      !pv_node &&
      !stack->isNullSearch &&
      static_eval != NINF;
	

    // 4. razoring    
    float rm = RazorMargin(depth);
    if (depth < 4 &&
        !pv_node &&
        ttm == MOVE_NONE &&
        !stack->isNullSearch &&
        static_eval + rm <= alpha &&
        !b.in_check() &&
        !b.pawn_on_7(b.whos_move())) {
      
      if (depth <= 1) {
        int v = qsearch<NONPV>(b, alpha, beta, 0, stack, false);
        if (v <= alpha) {
	  
	  if (debugging) {
	    stack->ni.razor_prune = true;
	    stack->ni.eval = alpha;
	    stack->ni.log_search_end(ofile);
	  }
	  
	  return alpha;
	}
      }
      else {
	int ralpha = alpha - rm;
	int v = qsearch<NONPV>(b, ralpha, ralpha + 1, depth, stack, false);
	if (v <= ralpha) {
	  
	  if (debugging) {
	    stack->ni.razor_prune = true;
	    stack->ni.eval = ralpha;
	    stack->ni.log_search_end(ofile);
	  }

	  return ralpha;
	}
      }
    }
        
    // 5. futility pruning
    
    if (// depth <= 2 &&
	depth <= 6 && 
	do_fwd_prune && 
        //static_eval - (650 - 100 * depth) >= beta &&
	static_eval - 50*depth >= beta && 
        beta < INF - mate_dist &&
        b.non_pawn_material(b.whos_move())) {
      
      if (debugging) {
	stack->ni.fwd_futility_prune = true;
	stack->ni.eval = beta;
	stack->ni.log_search_end(ofile);
      }
      
      return beta; // fail hard
    }
    
    
    // 6. null move search    
    if (do_fwd_prune &&
        depth >= 2 &&
        static_eval >= beta &&
        b.non_pawn_material(b.whos_move())) {
      
      int R = (depth >= 8 ? depth / 2 + (depth - 8) / 4 : 2);
      
      BoardData pd;
      
      (stack + 1)->isNullSearch = true;
      
      b.do_null_move(pd);

      int null_eval = (depth - R <= 1 ? -qsearch<NONPV>(b, -beta, -beta + 1, 0, stack + 1, false) : -search<NONPV>(b, -beta, -beta + 1, depth - R, stack + 1));
      
      b.undo_null_move();
      
      (stack + 1)->isNullSearch = false;
      

      if (null_eval >= beta) {
	if (debugging) {
	  stack->ni.null_mv_prune = true;
	  stack->ni.eval = null_eval;
	  stack->ni.log_search_end(ofile);
	}
	return beta;
      }
      
    }

    // 7. -- probcut from stockfish (disabled)
    if (!pv_node && 0 &&
        depth >= 6 &&
        !b.in_check() &&
        !stack->isNullSearch &&
        beta < MATE_IN_MAXPLY &&
	static_eval - 100 >= beta &&
        get_movetype((stack - 1)->currmove) == CAPTURE) {
      BoardData pd;
      MoveSelect ms(statistics, QsearchCaptures, false); // no checks just captures
      U16 move;
      int rdepth = depth / 2;
      int rbeta = beta + MAX(rdepth, 1) * 50;
      while (ms.nextmove(b, stack, ttm, move, false)) {
        if (!b.is_legal(move)) continue;
        b.do_move(pd, move);
        stack->currmove = move;
        eval = -search<NONPV>(b, -rbeta, -rbeta + 1, rdepth, stack + 1);
        b.undo_move(move);
        if (eval >= rbeta) return rbeta;
      }
    }

    // 7. -- internal iterative deepening
    if (ttm == MOVE_NONE && 
        depth >= (pv_node ? 8 : 6) &&
        (pv_node || static_eval + 250 >= beta) &&
        !b.in_check()) 
      {        
        int R = (depth >= 8 ? depth / 2 + (depth - 8) / 4 : 2);
        int iid = depth - R;
	
        stack->isNullSearch = true;
        
        if (pv_node) search<PV>(b, alpha, beta, iid, stack);	
        else search<NONPV>(b, alpha, beta, iid, stack);
        
        stack->isNullSearch = false;

	if (hashTable.fetch(key, e)) {
	  
	  if (debugging) {
	    stack->ni.iid_hit = true;
	  }
	  
	  ttm = e.move;
	}
      }

    // check if singular extension node
    /*
    singular_extension = !ROOT &&
      depth >= 8 &&
      ttm != MOVE_NONE &&
      e.bound == BOUND_LOW &&
      e.depth > depth - 3 &&
      excluded_move == MOVE_NONE && // no recursion
      ttvalue < INF - mate_dist && beta < INF - mate_dist;
    */
    // 6. -- moves search
    BoardData pd;
    MoveSelect ms(statistics, MainSearch);
    U16 move;
    int pruned = 0; moves_searched = 0;
    improving = static_eval - (stack - 2)->static_eval >= 0;

    
    while (ms.nextmove(b, stack, ttm, move, false)) {
      if (UCI_SIGNALS.stop) { return DRAW; }

      //if (move == excluded_move) continue;
	
      if (!b.is_legal(move)) {
        ++pruned;
        continue;
      }
      
      if (debugging) {
	stack->ni.eval = NINF;
	stack->ni.move = move;
	stack->ni.reduction = 0;
	stack->ni.main_futility_prune = false;
	stack->ni.see_prune = false;
	stack->ni.beta_prune = false;
	stack->ni.log_search_start(ofile);
      }
      
      // piece and move data
      bool givesCheck = b.gives_check(move); 
      stack->givescheck = givesCheck;
      bool inCheck = b.in_check();
      bool isQuiet = b.is_quiet(move);
      bool pvMove = (moves_searched == 0 && pv_node);
      int see_sign = b.see_sign(move);
      
      // see pruning at shallow depths      
      if (depth <= 3 && 
	  !pv_node &&
	  !ROOT &&
	  !pvMove &&
          !givesCheck &&
          isQuiet &&
          move != ttm &&
	  move != stack->killer[0] &&
	  move != stack->killer[1] &&
          !inCheck &&	  
          see_sign < 0) {
	++pruned;
	
	if (debugging) {
	  stack->ni.see_prune = true;
	  stack->ni.log_search_end(ofile);
	}
	
	continue;
      }
      

      // extension/reductions
      int extension = 0; int reduction = 1; // always reduce current depth by 1
      if (inCheck) {
	
	if (debugging) {
	  stack->ni.check_extension = true;	  
	}	
	extension += 1;
      }
      
      //if (threat_extension) extension += 1;

      // singular extension search (same implementation as stockfish)
      // TODO - tt-entries are currently spared by SE (hack)
      if (singular_extension && 0 &&
          move == ttm &&
          extension == 0)
        {
          int rbeta = beta - 2 * depth; // ttvalue - 2 * depth;
          stack->excludedMove = move;
          stack->isNullSearch = true; // turn off null-move pruning
          eval = search<NONPV>(b, rbeta - 1, rbeta, depth / 2, stack);
          stack->isNullSearch = false;
          stack->excludedMove = MOVE_NONE;
	  
          if (eval < rbeta) extension += 1;
        }

      if (depth >= 4 &&
          !inCheck &&
          !givesCheck &&
          isQuiet &&
          move != ttm &&
          move != stack->killer[0] &&
          move != stack->killer[1])
        {
          reduction += 1;
          if (depth > 6 && !pv_node) reduction += (depth - 6);

	  if (debugging) {
	    stack->ni.reduction = reduction;
	  }
	  
        }

      // adjust search depth based on reduction/extensions
      int newdepth = depth + extension - reduction;

      // futility pruning
      if (newdepth <= 1 &&
          move != ttm &&
          move != stack->killer[0] &&
          move != stack->killer[1] &&
          !inCheck &&
          !givesCheck &&
          isQuiet &&
          !pv_node &&
          eval + 650 < alpha &&
          eval > NINF + mate_dist) {

	if (debugging) {
	  stack->ni.main_futility_prune = true;
	  stack->ni.log_search_end(ofile);
	}

        //printf("!!DBG futility prune move(%s) depth(%d), alpha(%d) eval(%d) beta(%d), nodes(%d), msnodes(%d), qsnodes(%d)\n", (b.whos_move() == WHITE ? "white" : "black"), depth, alpha, eval, beta, b.get_nodes_searched(), b.MSnodes(), b.QSnodes());
        ++pruned;
        continue;
      }
      //printf("   !!DBG color(%s) domove(%s), alpha(%d) eval(%d) beta(%d), nodes(%d), msnodes(%d), qsnodes(%d)\n", (b.whos_move() == WHITE ? "white" : "black"), UCI::move_to_string(move).c_str(), alpha, eval, beta, b.get_nodes_searched(), b.MSnodes(), b.QSnodes());
      b.do_move(pd, move);

      // stack updates
      stack->currmove = move;
      stack->givescheck = givesCheck;

      bool fulldepthSearch = false;
      if (!pvMove &&
          move != ttm &&
          //move != stack->killer[0] &&
          //move != stack->killer[1] &&
          !givesCheck &&
          !inCheck &&
          isQuiet &&
	  
          //extension == 0 &&
          depth > 3 &&
          //!b.pawn_on_7(b.whos_move()) &&
          moves_searched > 1)
	{
	  int R = Reduction(pv_node, improving, newdepth, moves_searched)/2;
	  int v = statistics.history[b.whos_move()][get_from(move)][get_to(move)];
	  if (v <= (NINF- 1)) R += 1;
	  int LMR = newdepth - R;
	  eval = (LMR <= 1 ? -qsearch<NONPV>(b, -alpha - 1, -alpha, 0, stack + 1, givesCheck) : -search<NONPV>(b, -alpha - 1, -alpha, LMR, stack + 1));
	  //if (eval > alpha) fulldepthSearch = true;
	}
      else fulldepthSearch = !pvMove;

      if (debugging) {
	stack->ni.fulldepthsearch = true;
      }
      
      if (fulldepthSearch) {
        //printf("!!DBG fulldepthSearch(true) move(%s) newdepth(%d), alpha(%d) eval(%d) beta(%d), nodes(%d), msnodes(%d), qsnodes(%d)\n", (b.whos_move() == WHITE ? "white" : "black"), newdepth, alpha, eval, beta, b.get_nodes_searched(), b.MSnodes(), b.QSnodes());
        eval = (newdepth <= 1 ? -qsearch<NONPV>(b, -alpha - 1, -alpha, 0, stack + 1, givesCheck) : -search<NONPV>(b, -alpha - 1, -alpha, newdepth, stack + 1));
      }
      
      if (pvMove || eval > alpha) {
        //printf("!!DBG pvSearch(true) move(%s) newdepth(%d), alpha(%d) eval(%d) beta(%d), nodes(%d), msnodes(%d), qsnodes(%d)\n", (b.whos_move() == WHITE ? "white" : "black"), newdepth, alpha, eval, beta, b.get_nodes_searched(), b.MSnodes(), b.QSnodes());
        eval = (newdepth <= 1 ? -qsearch<PV>(b, -beta, -alpha, 0, stack + 1, givesCheck) : -search<PV>(b, -beta, -alpha, newdepth, stack + 1));
      }
      
      if (type == ROOT) {
        MoveList root(move, eval);
        if(!RootsContain(root)) RootMoves.push_back(root);
      } 

      b.undo_move(move);
      moves_searched++;
      if (UCI_SIGNALS.stop) return DRAW;

      // record move scores/evals
      if (eval >= beta) {

	if (debugging) {
	  stack->ni.beta_prune = true;
	  stack->ni.log_search_end(ofile);
	}
	
        if (excluded_move != MOVE_NONE) return beta;
        //printf("!!DBG (search) beta cut :: depth(%d) alpha(%d),eval(%d),beta(%d), bm=%s\n", depth, alpha, eval, beta, UCI::move_to_string(move).c_str());
        if (eval >= MATE_IN_MAXPLY && givesCheck) update_pv(stack->pv, move, (stack + 1)->pv); // mating move
        if (isQuiet) {
          if (quiets_searched < MAXDEPTH - 1) {
            quiets[quiets_searched++] = move;
            quiets[quiets_searched] = MOVE_NONE;
          }
          statistics.update(b,
                            move,
                            lastmove,
                            stack,
                            depth,
                            adjust_score(eval, mate_dist),
                            quiets);
        }
        hashTable.store(key,
                        data,
                        depth,
                        BOUND_LOW,
                        move,
                        adjust_score(eval, mate_dist), //beta, mate_dist),
                        static_eval,
                        pv_node);
        return beta;
      }
	
      if (eval > alpha) {
        //printf("!!DBG (search, eval>alpha) :: depth(%d) alpha(%d),eval(%d),beta(%d), bm=%s\n", depth, alpha, eval, beta, UCI::move_to_string(move).c_str());
        stack->bestmove = move;
        stack->givescheck = givesCheck;
        alpha = eval;
        bestmove = move;
        if (!stack->isNullSearch) update_pv(stack->pv, move, (stack + 1)->pv);
      }
    }

    if (b.in_check() && !moves_searched && (excluded_move == MOVE_NONE)) {
      //printf("!!DBG found mate (search) :: mate_dist(%d) alpha(%d),eval(%d),beta(%d), bm=%s\n", mate_dist, alpha, eval, beta, UCI::move_to_string(stack->bestmove).c_str());
      return NINF + mate_dist;
    }
    else if (b.is_draw(moves_searched) && excluded_move == MOVE_NONE) return DRAW;

    // update gui 
    //if (timer_thread->elapsed - last_time_ms >= 3000)
    //{
    //	pv_from_tt(ROOT_BOARD, eval, iter_depth);
    //	last_time_ms = timer_thread->elapsed;
    //}
    if (excluded_move != MOVE_NONE) return alpha;

    Bound ttb = alpha <= aorig ? BOUND_HIGH : alpha >= beta ? BOUND_LOW : BOUND_EXACT;
    hashTable.store(key, data, depth, ttb, bestmove, adjust_score(alpha, mate_dist), static_eval, pv_node);


    if (debugging) {
      stack->ni.eval = alpha;
      stack->ni.move = bestmove;
      stack->ni.fail_low = (alpha <= aorig);
      stack->ni.log_search_end(ofile);
    }
    return alpha;
  }
  
  template<NodeType type>
  int qsearch(Board& b, int alpha, int beta, int depth, Node* stack, bool inCheck)
  {
    int eval = NINF;
    //int ttval = NINF;
    //int ttstatic_value = NINF;

    U64 key = 0ULL;
    U64 data = 0ULL;

    TableEntry e;
    //bool root_node = type == ROOT;
    bool pv_node = (type == ROOT || type == PV);

    U16 ttm = MOVE_NONE;
    U16 bestmove = MOVE_NONE;
    
    stack->ply = (stack - 1)->ply++;
    //if (b.is_repition_draw()) return DRAW;

    int aorig = alpha;
    int mate_dist = iter_depth - depth; //stack->ply;
    //bool split = (b.get_worker()->currSplitBlock != NULL);
    //SplitBlock * split_point;
    //if (split) split_point = b.get_worker()->currSplitBlock;

    //U16 lastmove = (stack - 1)->currmove;
    bool genChecks = (stack - 2)->givescheck;// && depth > -3;
    //if (genChecks)
    //{
    //	b.print();
    //	printf("..prev-move = %s, tomv=%s\n", UCI::move_to_string((stack - 2)->currmove).c_str(), b.whos_move() == WHITE ? "white" : "black");
    //	
    //	//printf("..dbg genchecks = true\n");
    //}
    int qsDepth = (genChecks ? -1 : 0);

    // transposition table lookup    
    data = b.data_key();
    key = b.pos_key();
    if (hashTable.fetch(key, e)) {
      ++hash_hits;
      ttm = e.move;
      bool can_return = (e.pvNode || e.pvNode == pv_node);
      if (e.Depth() >= qsDepth && can_return) {
        if (e.bound == BOUND_EXACT && e.value > alpha && e.value < beta) return e.value;
        else if (e.bound == BOUND_LOW && e.value >= beta) return beta; 
        else if (e.bound == BOUND_HIGH && e.value <= alpha) return alpha; 
      }
    }

    // stand pat lower bound -- tried using static_eval for the stand-pat value, play was weak		
    int stand_pat = 0;
    bool kattacked = inCheck || b.in_check();
    
    if (!kattacked) stand_pat = Eval::evaluate(b);

    // note: what to do when ttval << stand_pat || ttval >> stand_pat (?)
    // in either case one result is innaccurate 
    // dbg check on evaluate vs. search vs. ttvalue
    //if (ttval > NINF && pv_node)
    //{
    //	//int eval2 = Eval::evaluate(b);
    //	
    //	//int iid = depth - 2;
    //	int seval = NINF;
    //	seval = search<PV>(b, alpha, beta, 1, stack, b.in_check());
    //	
    //	printf("..dbg ttval(%d), static-eval(%d), search-eval(%d), alpha(%d), beta(%d)\n", ttval, stand_pat, seval, alpha, beta);

    //}


    // delta pruning                 		
    if (stand_pat < alpha - 950 && !kattacked) {
      return alpha;
    }
    if (alpha < stand_pat && !kattacked) alpha = stand_pat;
    if (alpha >= beta && !kattacked) return beta;
    
    MoveSelect ms(statistics, QsearchCaptures, genChecks);
    int moves_searched = 0, pruned = 0;
    U16 move;

    while (ms.nextmove(b, stack, ttm, move, false)) {
      if (!b.is_legal(move)) {
        ++pruned;
        continue;
      }

      // move data
      bool checksKing = b.gives_check(move);
      stack->givescheck = checksKing;

      // futility pruning 
      /*
        if (!checksKing &&
        !inCheck &&
        move != ttm &&
        //!isQuiet && // if we are not in check, this is a capture
        !pv_node &&
        eval + material.material_value(b.piece_on(get_to(move)), END_GAME) >= beta) {
        eval += material.material_value(b.piece_on(get_to(move)), END_GAME);
        ++pruned;
        continue;
        }
      
      if (move != ttm &&
	  !checksKing &&
	  !inCheck &&
	  !pv_node &&
	  eval + 650 < alpha &&
	  eval > NINF + mate_dist) {
        ++pruned;
        continue;
      }
      */
      
      // prune captures which have see values < 0
      
      if (!inCheck
          && !pv_node
          && !checksKing
          && move != ttm
          && b.see_sign(move) <= 0)
        {
          ++pruned;
          continue;
        }
      
      
      BoardData pd;
      b.do_move(pd, move, true);
      eval = -qsearch<type>(b, -beta, -alpha, depth - 1, stack + 1, checksKing);
      b.undo_move(move);
      moves_searched++;

      if (eval >= beta) {
        hashTable.store(key, data, qsDepth, BOUND_LOW, move, adjust_score(eval, mate_dist), stand_pat, pv_node);
        return beta;
      }
      if (eval > alpha) {
        //printf("!!DBG (qsearch) pv-node :: depth(%d) alpha(%d),eval(%d),beta(%d), bm=%s\n", depth, alpha, eval, beta, UCI::move_to_string(move).c_str());

        alpha = eval;
        bestmove = move;
        stack->currmove = move;
        if (!stack->isNullSearch) update_pv(stack->pv, move, (stack + 1)->pv);
      }
    }

    // detect terminal node
    if (!moves_searched && b.in_check()) return NINF + mate_dist;
    else if (b.is_repition_draw()) return DRAW;

    Bound ttb = alpha <= aorig ? BOUND_HIGH : alpha >= beta ? BOUND_LOW : BOUND_EXACT;
    hashTable.store(key, data, qsDepth, ttb, bestmove, adjust_score(alpha, mate_dist), stand_pat, pv_node);

    return alpha;
  }

  void update_pv(U16* pv, U16& move, U16* child_pv) {
    for (*pv++ = move; *child_pv && *child_pv != MOVE_NONE;)
      {
        *pv++ = *child_pv++;
      }
    *pv = MOVE_NONE;
  }
  
  void pv_from_tt(Board b, int eval, int d, U16 * pv) {
    TableEntry e;  int j = 0; std::string pv_str = "";
    BoardData pd; BestMoves[0] = MOVE_NONE;

    if (b.is_legal(pv[0])) {
      BestMoves[0] = pv[0];
      BestMoves[1] = pv[1];
    }
      

    for (j = 0; hashTable.fetch(b.pos_key(), e) && e.move != MOVE_NONE && j < d; ++j) {
      if (b.is_legal(e.move)) {
	if (j < 2 && BestMoves[j] == MOVE_NONE) BestMoves[j] = e.move;
	pv_str += UCI::move_to_string(e.move) + " ";
	b.do_move(pd, e.move);
      }
      else break;
    }
    
    printf("info score cp %d depth %d seldepth %d nodes %d time %d pv ",
           eval,
           d,
           d,
           b.get_nodes_searched(),
           (int)timer_thread->elapsed);
    std::cout << pv_str << std::endl;
  }
  
  void ReadoutRootMoves(int depth) {
    for (unsigned int j = 0; j < RootMoves.size(); ++j) {
      U16 mv = RootMoves[j].m;
      printf("info depth %u currmove %s currmovenumber %u\n", 
             depth, 
             UCI::move_to_string(mv).c_str(), j + 1);
    }
  }
    
  int adjust_score(int bestScore, int ply) {
    return (bestScore >= MATE_IN_MAXPLY ? bestScore = bestScore - ply : bestScore <= MATED_IN_MAXPLY ? bestScore + ply : bestScore);
  } 
}
