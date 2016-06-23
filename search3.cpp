#include <cstring>
#ifdef DEBUG
#include <cassert>
#endif

#include "search.h"
#include "hashtable.h"
#include "material.h"
#include "threads.h"
#include "evaluate.h"
#include "move.h"
#include "moveselect.h"
#include "uci.h"

// globals
SignalsType UCI_SIGNALS;
std::vector<U16> RootMoves;

namespace
{
  int iter_depth = 0;
  int nb_time_allocs = 0; // measure large shifts in eval from one depth to the next, to adjust time allocation during search..

  MoveStats statistics;
  //U16 pv_line[MAXDEPTH];
  std::vector<int> searchEvals;
  
  template<NodeType type>
  int search(Board& b, int alpha, int beta, int depth, Node* stack);
  
  template<NodeType type>
  int qsearch(Board& b, int alpha, int beta, int depth, Node* stack, bool inCheck);
  
  int adjust_score(int bestScore, int ply);
  //void write_pv_to_tt(Board& b, U16 * pv, int depth);
  void update_pv(U16* pv, U16& move, U16* child_pv);
  //void checkMoreTime(Board& b, Node * stack);
};

namespace Search
{
  void run(Board& b, int dpth)
  {
    // init search params
    int alpha = NINF;
    int beta = INF;
    //int eval = NINF;
    int besteval = NINF;
    int wdelta = 16; // ~1/3 pawn value

    // search nodes init
    Node stack[128 + 4];
    std::memset(stack, 0, (128 + 4) * sizeof(Node));

    // clear search data
    iter_depth = 1;
    searchEvals.clear();
    nb_time_allocs = 0;

    // start the timer thread now
    timer_thread->searching = true;
    timer_thread->elapsed = 0;
    timer_thread->sleep_condition.signal();

    // main entry point for the fail-hard alpha-beta search
    // the main iterative deepening loop
    for (int depth = 1; depth <= dpth; depth += 1)
      {
	wdelta = 25;
	alpha = std::max(NINF, besteval - wdelta);
	beta = std::min(INF, besteval + wdelta);
	while (true)
	  {
	    if (UCI_SIGNALS.stop) break;
	    //else if (UCI_SIGNALS.timesUp) checkMoreTime(b, stack + 2);
	    statistics.init(); // move ordering of quiet moves
	    besteval = search<ROOT>(b, alpha, beta, depth, stack + 2);		
	    iter_depth++;
	    if (!UCI_SIGNALS.stop)
	      {
		// output the pv-line to the UI
		std::string pv_str = "";
		int j = 0;
		while (U16 m = (stack + 2)->pv[j])
		  {
		    if (m == MOVE_NONE) break;
		    pv_str += UCI::move_to_string(m) + " ";
		    if (j < 2) BestMoves[j] = m;
		    j++;
		  }
		searchEvals.push_back(besteval);
		
		printf("info score cp %d depth %d seldepth %d nodes %d time %d pv ",
		       besteval,
		       depth,
		       j,
		       b.get_nodes_searched(),
		       (int)timer_thread->elapsed);
		std::cout << pv_str << std::endl;
		
		// update alpha/beta in case of fail low/high
		if (besteval <= alpha)
		  {
		    alpha = std::max(alpha - wdelta, NINF);
		  }
		else if (besteval >= beta)
		  {
		    beta = std::min(beta + wdelta, INF);
		  }
		else break;
		wdelta += 0.5 * wdelta;
	      }
	  }
	
	//write_pv_to_tt(b, (stack+2)->pv, depth);
      }
  }
  
  void from_thread(Board& b, int alpha, int beta, int depth, Node& node)
  {
    search<SPLIT>(b, alpha, beta, depth, NULL);
  }
};


namespace
{
  template<NodeType type>
  int search(Board& b, int alpha, int beta, int depth, Node* stack)
  {
    int eval = NINF;
    int besteval = NINF;
    //int aorig = alpha;
    
    // node type
    //bool split_node = (type == SPLIT);
    //bool root_node = (type == ROOT);
    bool pv_node = (type == ROOT || type == PV);
    
    U16 quiets[MAXDEPTH];
    //for (int j=0; j<MAXDEPTH; ++j) quiets[j] = MOVE_NONE;
    int moves_searched = 0;
    int quiets_searched = 0;
    U16 bestmove = MOVE_NONE;
    //U16 threat = MOVE_NONE;

    // update stack info
    stack->ply = (stack - 1)->ply++;
    stack->currmove = stack->bestmove = (stack+2)->killer1 = (stack+2)->killer2 = MOVE_NONE;
    stack->givescheck = false;

    // update last move info
    U16 lastmove = (stack - 1)->currmove;
    //U16 lastbest = (stack - 1)->bestmove;

    // init ttable entries
    TableEntry e;
    U64 key = 0ULL;
    U64 data = 0ULL;
    U16 ttm = MOVE_NONE;
    int ttvalue = NINF;
    int ttstatic_value = NINF;

    if (UCI_SIGNALS.stop) return DRAW;

    // 1. -- ttable lookup 
    data = b.data_key();
    key = b.pos_key();
    
    if (hashTable.fetch(key, e) && e.depth >= depth)
      {
	ttm = e.move;
	ttstatic_value = e.static_value;	
	ttvalue = e.value;
	stack->currmove = stack->bestmove = e.move;
	if (e.bound == BOUND_EXACT && pv_node) return e.value;
	else if (e.bound == BOUND_LOW && pv_node) 
	  {
	    if (e.value >= beta && !b.in_check()) statistics.update(ttm, lastmove, stack, depth, b.whos_move(), quiets);
	    return e.value; 
	  }
	else if (e.bound == BOUND_HIGH && pv_node) return e.value;
      }
    
    // 2. -- mate distance pruning        
    int mate_val = INF - stack->ply;
    if (mate_val < beta)
      {
	beta = mate_val;
	if (alpha >= mate_val) return mate_val;
      }
    
    int mated_val = NINF + stack->ply;
    if (mated_val > alpha)
      {
	alpha = mated_val;
	if (beta <= mated_val) return mated_val;
      }
    //if (alpha >= beta) return mated_val;
    

    // 3. -- static evaluation of position    
    int static_eval = (ttvalue > NINF ? ttvalue : ttstatic_value > NINF ? ttstatic_value : Eval::evaluate(b));
    //if (besteval == NINF) besteval = static_eval;

    // 4. -- drop into qsearch if we are losing
    if (depth <= 4 &&
	!pv_node && ttm == MOVE_NONE &&
	!stack->isNullSearch &&
	static_eval + 600 <= alpha &&
	!b.in_check() &&
	!b.pawn_on_7(b.whos_move()))
      {
	int ralpha = alpha - 650;
	if (depth <= 1)
	  {
	    return qsearch<NONPV>(b, alpha, beta, depth, stack, false);
	  }
	int v = qsearch<NONPV>(b, ralpha, ralpha + 1, depth, stack, false);
	if (v <= ralpha)
	  {
	    return v;
	  }
      }

    // 5. -- futility pruning
    if (depth <= 5 && 
	!pv_node && !b.in_check() &&
	!stack->isNullSearch &&
	static_eval - 200*depth >= beta && 
	beta < INF - stack->ply &&
	b.non_pawn_material(b.whos_move()))
      {
	return static_eval - 200*depth; // fail soft
      }
	

    // 6. -- null move search    
    if (!pv_node &&
	depth >= 2 &&
	!stack->isNullSearch &&
	static_eval >= beta &&
	!b.in_check() &&
	b.non_pawn_material(b.whos_move()))
      {
	int R = (depth >= 8 ? depth / 2 : 2);  // really aggressive depth reduction	
	BoardData pd;	

	(stack + 1)->isNullSearch = true;	
	b.do_null_move(pd);
	int null_eval = (depth - R <= 1 ? -qsearch<NONPV>(b, -beta, -beta + 1, depth - R, stack + 1, false) : -search<NONPV>(b, -beta, -beta + 1, depth - R, stack + 1));
	b.undo_null_move();	
	(stack + 1)->isNullSearch = false;
	
	if(null_eval >= beta) return null_eval; // fail soft
      }

    // 7. -- probcut from stockfish
    /*
    if (!pv_node && 
	depth >= 500 && !b.in_check() &&
	!stack->isNullSearch)
      {
	BoardData pd;
	MoveSelect ms; // slow initialization method
	MoveGenerator mvs(b, LEGAL);
	U16 move; 
	int rbeta = beta + 200;
	int rdepth = depth - 4;
	ms.load(mvs, b, ttm, statistics, stack->killer1, stack->killer2, lastmove, stack->threat, stack->threat_gain);
	while (ms.nextmove(*stack, move, false))
	  {
	    b.do_move(pd, move);
	    stack->currmove = move;
	    eval = -search<NONPV>(b, -rbeta, -rbeta + 1, rdepth, stack + 1);
	    b.undo_move(move);
	    if (eval >= rbeta)
	      {
		//printf("..probcut prune\n");
		return beta; // fail hard.
	      }
	  }
      }
    */
    
    // 7. -- internal iterative deepening
    if (ttm == MOVE_NONE &&
	!stack->isNullSearch &&
	depth >= (pv_node ? 6 : 8) &&
	!b.in_check() )
      {
	int iid = depth - 2 - (pv_node ? 0 : depth / 4);
	stack->isNullSearch = true;
	if (pv_node)
	  {
	    search<PV>(b, alpha, beta, iid, stack);	    
	  }
	else
	  {
	    search<NONPV>(b, alpha, beta, iid, stack);
	  }
	stack->isNullSearch = false;
	if (hashTable.fetch(key,e)) ttm = e.move;
      }

    // 6. -- moves search
    BoardData pd;
    MoveSelect ms; // slow initialization method
    MoveGenerator mvs(b, PSEUDO_LEGAL);
    U16 move;
    if (b.is_draw(mvs.size())) return DRAW;
    
    ms.load(mvs, b, ttm, statistics, stack->killer1, stack->killer2, lastmove, stack->threat, stack->threat_gain);

    while (ms.nextmove(*stack, move, false))
      {
	if (UCI_SIGNALS.stop) return DRAW;
	if (move == MOVE_NONE || !b.is_legal(move)) continue;

	// piece and move data
	int piece = b.piece_on(get_from(move));
	bool givesCheck = b.checks_king(move);
	bool inCheck = b.in_check();
	bool isQuiet = b.is_quiet(move);
	bool pvMove = (moves_searched == 0 && pv_node);

	// extension/reductions
	int extension = 0; int reduction = 1; // always reduce current depth by 1
	if (inCheck) extension += 1; 

	if (depth >= 8 &&
	    !inCheck && !givesCheck && isQuiet &&
	    move != ttm &&
	    move != stack->killer1 &&
	    move != stack->killer2 )
	  {
	    reduction += 1;
	    if (depth > 10 && !pv_node) reduction += 1;
	  }

	// adjust search depth based on reduction/extensions
	int newdepth = depth + extension - reduction;

	// futility pruning
	if (newdepth <= 1 &&
	    move != ttm &&
	    move != stack->killer1 &&
	    move != stack->killer2 &&
	    !inCheck && !givesCheck && isQuiet && !pv_node &&
	    besteval < alpha && 
	    besteval > NINF + stack->ply )
	  {
	    continue;
	  }
	
	// exchange pruning at shallow depths - same thing done in qsearch...		    				
	if (newdepth <= 1 &&
	    !inCheck && !givesCheck && !isQuiet && //!pv_node &&
	    move != ttm &&
	    move != stack->killer1 &&
	    move != stack->killer2 &&
	    //eval < alpha && 
	    b.see_move(move) <= 0) 
	  {
	    continue;
	  }	
	
	// PVS-type search within a fail-hard framework
	b.do_move(pd, move);
	
	// stack updates
	stack->currmove = move;
	stack->givescheck = givesCheck;
	
	// idea : perform full window PV searches until we raise alpha, 
	// then null window NONPV searches, unless we raise alpha again
	if (pvMove)
	  {
	    eval = (newdepth <= 1 ? -qsearch<PV>(b, -beta, -alpha, newdepth, stack + 1, givesCheck) : -search<PV>(b, -beta, -alpha, newdepth, stack + 1));
	  }
	else
	  {
	    pvMove = false;
	    // LMR 
	    int R = 0;
	    if (move != ttm &&
		move != stack->killer1 &&
		move != stack->killer2 &&
		!inCheck && !givesCheck &&
		piece != PAWN &&
		quiets_searched > 0 &&
		newdepth > 10)
	      {
		R += 1;
		if (quiets_searched > 4) R += 1;
	      }	    
	    int LMR = newdepth - R;
	    eval = (LMR <= 1 ? -qsearch<NONPV>(b, -alpha-1, -alpha, LMR, stack + 1, givesCheck) : -search<NONPV>(b, -alpha-1, -alpha, LMR, stack + 1));
	  }
	if (!pvMove && (eval > alpha && eval < beta))
	  {
	    eval = (newdepth <= 1 ? -qsearch<PV>(b, -beta, -alpha, newdepth, stack + 1, givesCheck) : -search<PV>(b, -beta, -alpha, newdepth, stack + 1));
	  }
	
	b.undo_move(move);
	moves_searched++;
	if (UCI_SIGNALS.stop) return DRAW;

	if (isQuiet && quiets_searched < MAXDEPTH - 1)
	  {
	    quiets[quiets_searched++] = move;
	    quiets[quiets_searched] = MOVE_NONE;
	  }
	
	// record move scores/evals
	if (eval > besteval)
	  {
	    //printf("eval = %d, besteval = %d\n",eval, besteval);
	    besteval = eval;

	    if (eval > alpha)
	      {
		stack->bestmove = bestmove = move;
		if (pv_node && eval < beta) 
		  {
		    alpha = eval;
		    update_pv(stack->pv, move, (stack+1)->pv);
		  }
		else if (eval >= beta)
		  {
		    break;
		  }		
	      }
	  }
      } // end move loop

    /*
    if (!moves_searched)
      {
	besteval = b.in_check() ? NINF + stack->ply : 0; // must be draw/stalemate
      }
    */
    if (besteval >= beta && !b.in_check()) statistics.update(move, lastmove, stack, depth, b.whos_move(), quiets);

    // ttable updates
    Bound ttb = BOUND_NONE;
    if (pv_node && bestmove != MOVE_NONE) ttb = BOUND_EXACT;// && besteval < beta && besteval > alpha) ttb = BOUND_EXACT;
    else if (besteval >= beta) ttb = BOUND_LOW;
    else if (besteval <= alpha) ttb = BOUND_HIGH;    
    if (ttb != BOUND_NONE) hashTable.store(key, data, depth, ttb, bestmove, adjust_score(besteval, stack->ply), static_eval, iter_depth);

    return besteval;
  } // end search

  
  template<NodeType type>
  int qsearch(Board& b, int alpha, int beta, int depth, Node* stack, bool inCheck)
  {
    int eval = NINF;
    int ttval = NINF;
    int besteval = NINF;
    //int ttstatic_value = NINF;

    U64 key = 0ULL;
    U64 data = 0ULL;
    
    TableEntry e;
    bool pv_node = (type == ROOT || type == PV);
    
    U16 ttm = MOVE_NONE;
    U16 bestmove = MOVE_NONE;
    
    //int aorig = alpha;
    //bool split = (b.get_worker()->currSplitBlock != NULL);
    //SplitBlock * split_point;
    //if (split) split_point = b.get_worker()->currSplitBlock;
        
    stack->ply = (stack - 1)->ply++;
    //U16 lastmove = (stack - 1)->currmove;
    int moves_searched = 0;

    // transposition table lookup    
    data = b.data_key();
    key = b.pos_key();
    if (hashTable.fetch(key, e) && e.depth >= depth)
      {
	ttm = e.move;
	ttval = e.value;
	stack->currmove = stack->bestmove = e.move;
	if (e.bound == BOUND_EXACT && pv_node) return e.value;
	else if (e.bound == BOUND_LOW && pv_node) return e.value;  // update stats here !?
	else if (e.bound == BOUND_HIGH && pv_node) return e.value;
      }
    
    // stand pat lower bound -- tried using static_eval for the stand-pat value, play was weak
    int stand_pat = (ttval = NINF ? Eval::evaluate(b) : ttval);
    if (ttval == NINF) stand_pat = Eval::evaluate(b);
    besteval = stand_pat;

    if (stand_pat >= beta && !inCheck) return stand_pat; 
    if (alpha < stand_pat && pv_node && !inCheck) alpha = stand_pat;
    if (alpha >= beta && !inCheck) return alpha; // fail soft
       
    // searching only checks or captures causes it to miss obv. "forcing moves" at higher depths ..
    MoveGenerator mvs(b,inCheck);
    //MoveGenerator mvs(b, CAPTURE); // play is a little stronger with this one
		
    // mate detection
    if (inCheck && mvs.size() == 0) return NINF + stack->ply;
    
    MoveSelect ms;
    U16 killer = MOVE_NONE;
    ms.load(mvs, b, ttm, statistics, killer, killer, killer, killer, 0);
    U16 move;
    Node dummy;

    // movegenerator generates only legal moves in qsearch (fyi)
    while (ms.nextmove(dummy, move, false))
      {
	if (move == MOVE_NONE) continue;
	
	// move data
	int piece = b.piece_on(get_from(move));
	bool givesCheck = b.checks_king(move); // working ok
	bool isQuiet =  b.is_quiet(move); // evasions

	// prune evasions
	bool canPrune =  (inCheck && !givesCheck && isQuiet &&
			  move != ttm && !pv_node);
	//if (canPrune) continue;
	
	// futility pruning --continue if we are winning	
	if (!givesCheck && !inCheck && 
	    move != ttm && !pv_node &&
	    piece != PAWN && 	      
	    besteval + material.material_value(b.piece_on(get_to(move)), END_GAME ) >= beta &&
	    b.see_move(move) >= 0)
	  {
	    //printf("%d %d %d", alpha, beta, eval);
	    //b.print();
	    //eval = alpha - fv;
	    //continue;	      
	    eval += material.material_value(b.piece_on(get_to(move)), END_GAME );
	    continue;
	  }	
	
	// prune captures which have see values <= 0	  
	if ( (!inCheck || canPrune) &&
	     !pv_node && //move != ttm &&
	     b.see_move(move) < 0)
	  continue;
	
	BoardData pd;	
	b.do_move(pd, move);	
	eval = -qsearch<type>(b, -beta, -alpha, depth - 1, stack + 1, givesCheck);
	b.undo_move(move);	
	moves_searched++;
	
	// record best move update
	if (eval > besteval)
	  {
	    besteval = eval;
	    if (eval > alpha)
	      {
		if (pv_node && eval < beta) 
		  {
		    alpha = eval;
		    bestmove = move;
		  }
		else if (eval >= beta)
		  {
		    hashTable.store(key, data, depth, BOUND_LOW, bestmove, adjust_score(eval, stack->ply), stand_pat, iter_depth);
		    return eval;
		  }		
	      }
	  }
      
      } // end moves loop
    
    /*
    if (inCheck && besteval == NINF)
      {
	return NINF + stack->ply;
      }
    */
    
    // ttable updates
    Bound ttb = BOUND_NONE;
    //if (pv_node && bestmove != MOVE_NONE && besteval < beta && besteval > alpha) ttb = BOUND_EXACT;
    if (pv_node && bestmove != MOVE_NONE) ttb = BOUND_EXACT;
    else if (besteval >= beta) ttb = BOUND_LOW;
    else if (besteval <= alpha) ttb = BOUND_HIGH;    
    if (ttb != BOUND_NONE) hashTable.store(key, data, depth, ttb, bestmove, adjust_score(besteval, stack->ply), stand_pat, iter_depth);

    return besteval;
  }

  void update_pv(U16* pv, U16& move, U16* child_pv)
  {
    for (*pv++ = move; *child_pv && *child_pv != MOVE_NONE;)
      {
	*pv++ = *child_pv++;
      }
    *pv = MOVE_NONE;
  }

  int adjust_score(int bestScore, int ply)
  {
    return (bestScore >= MATE_IN_MAXPLY ? bestScore = bestScore - ply :
	    bestScore <= MATED_IN_MAXPLY ? bestScore + ply : bestScore);
  }
};

