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
#include "opts.h"
#include "book.h"
#include "pgnio.h"

// globals
SignalsType UCI_SIGNALS;
std::vector<U16> RootMoves;

namespace
{
	int iter_depth = 0;
	U64 last_time_ms = 0; // print pv frequently

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
	void pv_from_tt(Board& b, int eval, int d);
	void store_pv(Board& b, U16 * pv, int depth);
	void print_pv_info(Board& b, int depth, int eval, U16 * pv);
};

namespace Search
{
	void run(Board& b, int dpth, bool pondering)
	{
		// check opening table if using opening book
		if (options["opening book"] && b.half_moves() <= 16)
		{
#ifdef WIN32
			pgn_io pgn("A:\\software\\chess\\engine\\hedwig-git\\chess\\x64\\Release\\testdb.bin");
#else
			pgn_io pgn("/home/mjg/projects/chess-code/hedwig-git/chess/testdb.bin");
#endif
			printf("...using opening book \n");
			std::string bookmove = pgn.find(b.to_fen().c_str());
			if (bookmove != "")
			{
				U16 m = UCI::get_move(bookmove);
				if (m != MOVE_NONE)
				{
					BestMoves[0] = m; BestMoves[1] = MOVE_NONE;
					return;
				}
			}
		}
		else printf("..not using opening book\n");

		// catch ponder hit
		if (options["pondering"] && !pondering && PonderMoves[0] != MOVE_NONE)
		{
			// fixme .. did the opponent make the predicted bestmove
			if (b.piece_on(get_from(BestMoves[1])) == PIECE_NONE &&
				b.piece_on(get_to(BestMoves[1])) != PIECE_NONE)
			{
				BestMoves[0] = PonderMoves[0]; BestMoves[1] = MOVE_NONE;
				PonderMoves[0] = MOVE_NONE; PonderMoves[1] = MOVE_NONE;
				return;
			}
		}
    // init search params
    int alpha = NINF;
    int beta = INF;
    //int eval = NINF;
	int besteval = NINF; int prevEval = NINF;
    int wdelta = 50; // ~1/3 pawn value

    // search nodes init
    Node stack[128 + 4];
    std::memset(stack, 0, (128 + 4) * sizeof(Node));

    // clear search data
    iter_depth = 1;
    searchEvals.clear();

    // start the timer thread now
    timer_thread->searching = true;
    timer_thread->elapsed = 0;
    timer_thread->sleep_condition.signal();

	// main entry point for the fail-hard alpha-beta search
	// the main iterative deepening loop
	for (int depth = 1; depth <= dpth; depth += 1)
	{
		if (UCI_SIGNALS.stop) return;
		statistics.init(); // move ordering of quiet moves
		wdelta = 50;
		if (depth > 1)
		{
			alpha = max(NINF, prevEval - wdelta);
			beta = min(INF, prevEval + wdelta);
		}
		else { alpha = NINF; beta = INF; }
		

		while (true)
		{
			
			//else if (UCI_SIGNALS.timesUp) checkMoreTime(b, stack + 2);
			besteval = search<ROOT>(b, alpha, beta, depth, stack + 2);
			iter_depth++;
			if (UCI_SIGNALS.stop) break;
		
			// update alpha/beta in case of fail low/high
			if (besteval <= alpha)
			{
				alpha = max(alpha - wdelta, NINF);
			}
			else if (besteval >= beta)
			{
				beta = min(beta + wdelta, INF);
			}
			else break;
			wdelta += 0.5 * wdelta;
		}

		// output the pv-line to the UI
		//print_pv_info(b, depth, besteval, (stack + 2)->pv);
		pv_from_tt(b, besteval, depth);
		last_time_ms = timer_thread->elapsed;
		prevEval = besteval;
      }
  }
  
  void from_thread(Board& b, int alpha, int beta, int depth, Node& node)
  {
    search<SPLIT>(b, alpha, beta, depth, NULL);
  }
};


namespace
{
	int Reduction(bool pv_node, bool improving, int d, int mc)
	{
		return Globals::SearchReductions[(int)pv_node][(int)improving][min(d, 63)][min(mc, 63)];
	}
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
	bool root_node = (type == ROOT);

    U16 quiets[MAXDEPTH];
    //for (int j=0; j<MAXDEPTH; ++j) quiets[j] = MOVE_NONE;
    int moves_searched = 0;
    int quiets_searched = 0;
    U16 bestmove = MOVE_NONE;
    //U16 threat = MOVE_NONE;

    // update stack info
    stack->ply = (stack - 1)->ply++;
	stack->currmove = stack->bestmove = (stack + 2)->killer[0] = (stack + 2)->killer[1] = (stack + 2)->killer[2] = (stack + 2)->killer[3] = MOVE_NONE;
	stack->givescheck = false;

    // update last move info
    U16 lastmove = (stack - 1)->currmove;
    //U16 lastbest = (stack - 1)->bestmove;

	// clear pv
	for (int j = 0; stack->pv[j] != MOVE_NONE; ++j) stack->pv[j] = MOVE_NONE;

	// reductions array specific
	bool improving = false;

    // init ttable entries
    TableEntry e;
    U64 key = 0ULL;
    U64 data = 0ULL;
    U16 ttm = MOVE_NONE;
    int ttvalue = NINF;
    int ttstatic_value = NINF;
	int mate_dist = iter_depth - depth;

	// 1. -- mate distance pruning        
	int mate_val = INF - mate_dist;
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

	// 2. -- ttable lookup 
	data = b.data_key();
	key = b.pos_key();

	if (!root_node && 
		hashTable.fetch(key, e) && 
		e.depth >= depth)
	{
		ttm = e.move; 
		//ttstatic_value = e.static_value;
		ttvalue = e.value;
		stack->currmove = stack->bestmove = e.move;
		if (e.bound == BOUND_EXACT && pv_node) return e.value;
		else if (e.bound == BOUND_LOW && pv_node)
		{
			if (e.value >= beta && !b.in_check())
				statistics.update(b, ttm, lastmove, stack, depth, b.whos_move(), quiets);
			return e.value;
		}
		else if (e.bound == BOUND_HIGH && pv_node) return e.value;
	}   

    // 3. -- static evaluation of position    
    //int static_eval = (ttvalue > NINF ? ttvalue : ttstatic_value > NINF ? ttstatic_value : Eval::evaluate(b));
	//int static_eval = (ttvalue > NINF ? ttvalue : Eval::evaluate(b));
	int static_eval = Eval::evaluate(b);

    // 4. -- drop into qsearch if we are losing
	if (depth <= 4 &&
		!pv_node && ttm == MOVE_NONE &&
		!stack->isNullSearch &&
		static_eval + 650 <= alpha &&
		!b.in_check() &&
		!b.pawn_on_7(b.whos_move()))
	{
		int ralpha = alpha - 650;
		if (depth <= 1)
		{
			return qsearch<NONPV>(b, alpha, beta, 0, stack, false);
		}
		int v = qsearch<NONPV>(b, ralpha, ralpha + 1, depth, stack, false);
		if (v <= ralpha)
		{
			return v; // fail soft
		}
	}

    // 5. -- futility pruning
	if (depth <= 6 &&
		!pv_node && !b.in_check() &&
		!stack->isNullSearch &&
		static_eval - 200 * depth >= beta &&
		beta < INF - mate_dist &&
		b.non_pawn_material(b.whos_move()))
	{
		return static_eval - 200 * depth; // fail soft
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
		int null_eval = (depth - R <= 1 ? -qsearch<NONPV>(b, -beta, -beta + 1, 0, stack + 1, false) : -search<NONPV>(b, -beta, -beta + 1, depth - R, stack + 1));
		b.undo_null_move();
		(stack + 1)->isNullSearch = false;

		if (null_eval >= beta) return null_eval; // fail soft
	}

    // 7. -- probcut ?

    // 8. -- internal iterative deepening
	if (ttm == MOVE_NONE &&
		//!stack->isNullSearch &&
		depth >= (pv_node ? 6 : 8) &&
		(pv_node || static_eval + 250 >= beta) &&
		!b.in_check())
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
		if (hashTable.fetch(key, e)) ttm = e.move;
	}

    // 9. -- moves search
	BoardData pd;
	MoveSelect ms(statistics, MAIN);
	U16 move;
	int pruned = 0; moves_searched = 0;
	//if (ttm == MOVE_NONE) ttm = (stack-2)->pv[iter_depth-depth];

	improving = static_eval - (stack - 2)->static_eval >= 0 || static_eval == 0 || (stack - 2)->static_eval == 0;

	while (ms.nextmove(b, stack, ttm, move, false))
	{
		if (UCI_SIGNALS.stop) { return DRAW; }

		if (!b.is_legal(move))
		{
			++pruned;
			continue;
		}
		// piece and move data
		int piece = b.piece_on(get_from(move));
		bool givesCheck = b.gives_check(move);
		bool inCheck = b.in_check();
		bool isQuiet = b.is_quiet(move);
		bool pvMove = (moves_searched == 0 && pv_node);

		// extension/reductions
		int extension = 0; int reduction = 1; // always reduce current depth by 1
		if (inCheck) extension += 1;

		if (depth >= 8 &&
			!inCheck && !givesCheck && isQuiet &&
			move != ttm &&
			move != stack->killer[0] &&
			move != stack->killer[1])
		{
			reduction += 1;
			if (depth > 10 && !pv_node) reduction += 1;
		}

		// adjust search depth based on reduction/extensions
		int newdepth = depth + extension - reduction;

		// futility pruning
		if (newdepth <= 1 &&
			//moves_searched > 0 && quiets_searched > 0 &&
			move != ttm &&
			move != stack->killer[0] &&
			move != stack->killer[1] &&
			!inCheck && !givesCheck && isQuiet && !pv_node &&
			//static_eval + 650 < alpha && 
			eval + 650 < alpha &&
			eval > NINF + mate_dist)
		{
			++pruned;
			continue;
		}

		// exchange pruning at shallow depths - same thing done in qsearch...		    				
		if (newdepth <= 1 &&
			!inCheck && !givesCheck && //!isQuiet && // !pv_node &&
			!pv_node &&
			move != ttm &&
			move != stack->killer[0] &&
			move != stack->killer[1] &&
			//eval < alpha && 
			b.see_move(move) < 0)
		{
			++pruned;
			continue;
		}

		// PVS-type search within a fail-hard framework
		b.do_move(pd, move);

		// stack updates
		stack->currmove = move;
		stack->givescheck = givesCheck;

		bool fulldepthSearch = false;
		if (depth >= 2 &&
			!pvMove &&
			move != ttm &&
			move != stack->killer[0] &&
			move != stack->killer[1] &&
			!givesCheck && !inCheck &&
			isQuiet)
		{
			int R = Reduction(pv_node, improving, newdepth, moves_searched);

			int v = statistics.history[b.whos_move()][get_from(move)][get_to(move)];
			if (v <= (NINF - 1)) R += 1;

			int LMR = newdepth - R;
			eval = (LMR <= 1 ? -qsearch<NONPV>(b, -alpha - 1, -alpha, 0, stack + 1, givesCheck) : -search<NONPV>(b, -alpha - 1, -alpha, LMR, stack + 1));

			if (eval > alpha || eval > besteval) fulldepthSearch = true;
		}
		else fulldepthSearch = !pvMove;

		if (fulldepthSearch)
		{
			eval = (newdepth <= 1 ? -qsearch<NONPV>(b, -alpha - 1, -alpha, 0, stack + 1, givesCheck) : -search<NONPV>(b, -alpha - 1, -alpha, newdepth, stack + 1));
		}


		if (pvMove || (eval > alpha && eval < beta) )
		{
			eval = (newdepth <= 1 ? -qsearch<PV>(b, -beta, -alpha, 0, stack + 1, givesCheck) : -search<PV>(b, -beta, -alpha, newdepth, stack + 1));
		}

		b.undo_move(move);
		moves_searched++;
		if (UCI_SIGNALS.stop) return DRAW;

		// record move scores/evals
		if (eval >= beta)
		{
			if (isQuiet && quiets_searched < MAXDEPTH - 1)
			{
				quiets[quiets_searched++] = move;
				quiets[quiets_searched] = MOVE_NONE;
			}
			statistics.update(b, move, lastmove, stack, depth, b.whos_move(), quiets);
			hashTable.store(key, data, depth, BOUND_LOW, move, adjust_score(eval, mate_dist), static_eval, pv_node);
			return eval;
		}

		if (eval > besteval)
		{			
			besteval = eval;
			if (eval > alpha)
			{
				//update_pv(stack->pv, bestmove, (stack + 1)->pv);
				bestmove = move;
				alpha = eval;
			}
		}

	}

	if (!moves_searched)
	{
		if (b.in_check()) //(pruned != mvs.size() && b.in_check()) 
		{
			return NINF + mate_dist;
		}
		else if (b.is_draw(0)) //(pruned != moves_searched && b.is_draw(0)) 
		{
			return DRAW;
		}
	}
	// detect repetition draw
	else if (b.is_repition_draw()) return DRAW;

	// ttable updates
    Bound ttb = BOUND_NONE;
    if (pv_node && bestmove != MOVE_NONE) ttb = BOUND_EXACT;// && besteval < beta && besteval > alpha) ttb = BOUND_EXACT;
    else if (besteval >= beta ) ttb = BOUND_LOW; // already caught this one
    else ttb = BOUND_HIGH;
    if (ttb != BOUND_NONE) hashTable.store(key, data, depth, ttb, bestmove, adjust_score(besteval, mate_dist), static_eval, iter_depth);

    return besteval;
  } // end search

  
  template<NodeType type>
  int qsearch(Board& b, int alpha, int beta, int depth, Node* stack, bool givesCheck)
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
	int mate_dist = iter_depth - depth;
	bool inCheck = b.in_check();

    //int aorig = alpha;
    //bool split = (b.get_worker()->currSplitBlock != NULL);
    //SplitBlock * split_point;
    //if (split) split_point = b.get_worker()->currSplitBlock;
        
    stack->ply = (stack - 1)->ply++;
    //U16 lastmove = (stack - 1)->currmove;

    // transposition table lookup    
	data = b.data_key();
	key = b.pos_key();
	if (hashTable.fetch(key, e) && e.depth >= depth)
	{
		ttm = e.move; 
		ttval = e.value;
		stack->currmove = stack->bestmove = e.move;
		if (e.bound == BOUND_EXACT && pv_node) return e.value;
		else if (e.bound == BOUND_LOW && pv_node) return e.value;
		else if (e.bound == BOUND_HIGH && pv_node) return e.value;
	}
    
    // stand pat lower bound -- tried using static_eval for the stand-pat value, play was weak
    //int stand_pat = (ttval = NINF ? Eval::evaluate(b) : ttval);
    //if (ttval == NINF) stand_pat = Eval::evaluate(b);
	int stand_pat = Eval::evaluate(b);
	besteval = stand_pat;

    if (stand_pat >= beta && !inCheck) return stand_pat; 
	if (stand_pat < alpha - 950 && !inCheck)
	{
		return stand_pat; // fail soft
	}
	if (alpha < stand_pat && !inCheck) alpha = stand_pat;
    if (alpha >= beta && !inCheck) return alpha; // fail soft
       
	MoveSelect ms(statistics, QSEARCH, givesCheck);
	int moves_searched = 0, pruned = 0;
	U16 move;
	Node dummy;

	// movegenerator generates only legal moves in qsearch (fyi)
	while (ms.nextmove(b, stack, ttm, move, false))
	{
		if (!b.is_legal(move))
		{
			++pruned;
			continue;
		}

		// move data
		int piece = b.piece_on(get_from(move));
		bool checksKing = b.gives_check(move);
		bool isQuiet = b.is_quiet(move); // evasions

		// futility pruning --continue if we are winning	
		if (!checksKing && !inCheck &&//|| (givesCheck && depth < -3)) && 
			move != ttm &&
			!pv_node &&
			//piece != PAWN && 	      
			eval + material.material_value(b.piece_on(get_to(move)), END_GAME) >= beta &&
			b.see_move(move) >= 0)
		{
			eval += material.material_value(b.piece_on(get_to(move)), END_GAME);
			++pruned;
			continue;
		}

		// prune captures which have see values <= 0	  
		if (!pv_node && 
			!checksKing && !inCheck &&
			b.see_move(move) <= 0)
		{
			++pruned;
			continue;
		}

		BoardData pd;
		b.do_move(pd, move);
		eval = -qsearch<type>(b, -beta, -alpha, depth - 1, stack + 1, (depth > -1 ? givesCheck : false));
		b.undo_move(move);
		moves_searched++;

		// record best move update
		if (eval >= beta)
		{
			hashTable.store(key, data, depth, BOUND_LOW, move, adjust_score(eval, mate_dist), stand_pat, pv_node);
			return eval;
		}

		if (eval > besteval)
		{
			besteval = eval;
			if (eval > alpha)
			{
				//update_pv(stack->pv, bestmove, (stack + 1)->pv);
				bestmove = move;
				alpha = eval;
			}
		}
	} // end moves loop
    
	if (!moves_searched)
	{
		if (b.in_check()) //(pruned != mvs.size() && b.in_check()) 
		{
			return NINF + mate_dist;
		}
	}
	// detect repetition draw
	else if (b.is_repition_draw()) return DRAW;
    
    // ttable updates
    Bound ttb = BOUND_NONE;
    if (pv_node && bestmove != MOVE_NONE) ttb = BOUND_EXACT;
    else if (besteval >= beta ) ttb = BOUND_LOW;
	else ttb = BOUND_HIGH;
    if (ttb != BOUND_NONE) hashTable.store(key, data, depth, ttb, bestmove, adjust_score(besteval, mate_dist), stand_pat, iter_depth);

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

  void pv_from_tt(Board& b, int eval, int d)
  {
	  TableEntry e;  int j = 0; std::string pv = "";
	  std::vector<U16> moves;
	  for (j = 0; hashTable.fetch(b.pos_key(), e) && e.move != MOVE_NONE && j < d; ++j)
	  {
		  if (!b.is_legal(e.move)) break;
		  BoardData * pd = new BoardData();
		  if (j < 2 ) BestMoves[j] = e.move;
		  pv += UCI::move_to_string(e.move) + " ";
		  b.do_move(*pd, e.move); moves.push_back(e.move);
	  }
	  while (--j >= 0) b.undo_move(moves[j]);

	  printf("info score cp %d depth %d seldepth %d nodes %d time %d pv ",
		  eval,
		  d,
		  d,
		  b.get_nodes_searched(),
		  (int)timer_thread->elapsed);
	  std::cout << pv << std::endl;
  }

  void print_pv_info(Board& b, int depth, int eval, U16 * pv)
  {
	  std::string pv_str = "";
	  int j = 0;
	  while (U16 m = pv[j])
	  {
		  if (m == MOVE_NONE) break;
		  pv_str += UCI::move_to_string(m) + " ";
		  if (j < 2) BestMoves[j] = m;//(!pondering ? BestMoves[j] = m : PonderMoves[j] = m);
		  j++;
	  }
	  printf("info score cp %d depth %d seldepth %d nodes %d time %d pv ",
		  eval,
		  depth,
		  j,
		  b.get_nodes_searched(),
		  (int)timer_thread->elapsed);
	  std::cout << pv_str << std::endl;
  }

  void store_pv(Board& b, U16 * pv, int depth)
  {
	  /*
	  TableEntry e; int j = 0;
	  for (; pv[j] != MOVE_NONE; ++j)
	  {

	  U64 k = b.pos_key(); U64 data = b.data_key();
	  BoardData * pd = new BoardData();

	  if (!hashTable.fetch(k, e))
	  {
	  hashTable.store(k, data, depth, BOUND_NONE, pv[j], DRAW, DRAW);
	  }
	  b.do_move(*pd, pv[j]);
	  }
	  while(--j >= 0) b.undo_move(pv[j]);
	  */
  }

  int adjust_score(int bestScore, int ply)
  {
	  return (bestScore >= MATE_IN_MAXPLY ? bestScore = bestScore - ply : bestScore <= MATED_IN_MAXPLY ? bestScore + ply : bestScore);
  }
};

