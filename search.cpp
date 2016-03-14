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
	int min_split_depth = 0;
	int splits_per_thread = 0;
	int nb_time_allocs = 0; // idea == to measure large shifts in eval from one depth to the next, to adjust time allocation during search..
	MoveStats statistics;
	U16 pv_line[MAXDEPTH];
	std::vector<int> searchEvals;

	template<NodeType type>
	int search(Board& b, int alpha, int beta, int depth, Node* stack);

	template<NodeType type>
	int qsearch(Board& b, int alpha, int beta, int depth, Node* stack, bool inCheck);

	int adjust_score(int bestScore, int ply);
	void write_pv_to_tt(Board& b, U16 * pv, int depth);
	void update_pv(U16* pv, U16& move, U16* child_pv);
	void checkMoreTime(Board& b, Node * stack);
};

namespace Search
{

	void run(Board& b, int dpth)
	{
		min_split_depth = 4;// opts["MinSplitDepth"];
		splits_per_thread = 8;// opts["MaxSplitsPerThread"];

		iter_depth = 1;

		// init search params
		int alpha = NINF;
		int beta = INF;
		int eval = NINF;

		// search info stack
		Node stack[128 + 4];
		std::memset(stack, 0, (128 + 4) * sizeof(Node));

		// time allocation stats - used to decide if the search should be extended
		// in critical situations
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
			if (UCI_SIGNALS.stop) break;
			//else if (UCI_SIGNALS.timesUp) checkMoreTime(b, stack + 2);

			eval = search<ROOT>(b, alpha, beta, depth, stack + 2);

			iter_depth++;

			// print UCI info at the end of each search.
			if (!UCI_SIGNALS.stop)
			{
				// output the pv-line to the UI
				std::string pv_str = "";
				int j = 0;
				while (U16 m = (stack + 2)->pv[j])
				{
					if (m == MOVE_NONE) break;
					pv_str += UCI::move_to_string(m) + " ";// SanSquares[get_from(m)] + SanSquares[get_to(m)] + " ";
					if (j < 2) BestMoves[j] = m;
					j++;
				}
				searchEvals.push_back(eval);

				printf("info score cp %d depth %d seldepth %d nodes %d time %d pv ",
					eval,
					depth,
					j,
					b.get_nodes_searched(),
					(int)timer_thread->elapsed);
				std::cout << pv_str << std::endl;
			}

			// write the pv to the tt in case it was overwritten during the 
			// search
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
	//int MTDF(Board& b, int guess, int depth, Node * stack)
	//{
	//	// initialize mtdf parameters   
	//	int alpha = NINF;
	//	int beta = INF;
	//	int delta = INF;
	//	do
	//	{
	//		beta = (guess == alpha ? guess + 1 : guess);
	//		delta = 1;//(beta - alpha)/2;

	//		guess = search<ROOT>(b, beta - delta, beta, depth, stack);

	//		if (guess < beta) beta = guess;
	//		else alpha = guess;

	//	} while (alpha < beta);

	//	return guess;
	//}

	template<NodeType type>
	int search(Board& b, int alpha, int beta, int depth, Node* stack)
	{
		// initialize this node
		int eval = NINF;
		int aorig = alpha;

		// the node type
		bool split_node = (type == SPLIT);
		bool root_node = (type == ROOT);
		bool pv_node = (type == ROOT || type == PV);

		U16 quiets[MAXDEPTH];
		int moves_searched = 0;
		int quiets_searched = 0;

		// update the search stack at this depth
		stack->ply = (stack - 1)->ply++;
		U16 lastmove = (stack - 1)->currmove;

		// update the move history (counters/follow ups)
		U16 bestmove = MOVE_NONE;
		U16 threat = MOVE_NONE;

		// initialize the transposition table entries
		TableEntry e;
		U64 key = 0ULL;
		U64 data = 0ULL;
		U16 ttm = MOVE_NONE;
		int ttvalue = NINF;
		int ttstatic_value = NINF;

		// split data for parallel search
		Thread* worker = b.get_worker();
		SplitBlock * split_point = NULL;

		// check first if we are out of time or need to stop the search
		if (UCI_SIGNALS.stop) return DRAW;
		//else if (UCI_SIGNALS.timesUp) checkMoreTime(b, stack);


		if (split_node)
		{
			split_point = stack->sb;
			bestmove = split_point->bestmove;
			alpha = split_point->alpha;
			//aorig = split_point->alpha;
			//child->sb = NULL;
		}


		// transposition table lookup for this node
		if (!split_node)
		{
			data = b.data_key();
			key = b.pos_key();

			if (hashTable.fetch(key, e) && e.depth >= depth)
			{
				ttm = e.move;
				ttstatic_value = e.static_value;

				if (e.bound == BOUND_EXACT && e.value > alpha && e.value < beta)
				{
					stack->currmove = e.move;
					return e.value;
				}

				else if (e.bound == BOUND_LOW && !pv_node) return e.value;

				else if (e.bound == BOUND_HIGH && !pv_node) return e.value;

				if (alpha >= beta) return (int)beta;
				ttvalue = e.value;
			}
		}
		//if (type==ROOT && ttm == MOVE_NONE && stack->pv[0] != MOVE_NONE) ttm = stack->pv[0];


		// mate distance pruning
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
		if (alpha >= beta) return alpha;


		int static_eval = (ttvalue > NINF ? ttvalue : ttstatic_value > NINF ? ttstatic_value : Eval::evaluate(b));


		// stockfish style razoring	 -- if we are losing
		if (!pv_node &&
			depth <= 4 &&
			!b.in_check() &&
			ttm == MOVE_NONE &&
			!stack->isNullSearch &&
			!split_node &&
			static_eval + 600 <= alpha)
		{
			int ralpha = alpha - 600;
			int v = qsearch<NONPV>(b, ralpha, ralpha + 1, 1, stack, false);
			if (v <= ralpha)
				return v;
		}

		// stockfish style futility pruning	-- if we are winning			
		if (!pv_node &&
			depth <= 4 &&
			static_eval - 200 * depth >= beta &&
			!stack->isNullSearch &&
			beta < INF - stack->ply &&
			!b.in_check() &&
			!split_node)
			return beta;// fail hard


		// null move pruning
		if (!pv_node &&
			depth >= 2 &&
			!stack->isNullSearch &&
			static_eval >= beta &&
			!split_node &&
			!b.in_check() &&
			b.non_pawn_material(b.whos_move()))
		{
			int R = depth >= 8 ? depth / 2 : 2;  // really aggressive depth reduction

			BoardData pd;

			(stack + 1)->isNullSearch = true;

			b.do_null_move(pd);
			int null_eval = (depth - R <= 1 ? -qsearch<NONPV>(b, -beta, -beta + 1, depth - R, stack + 1, false) : -search<NONPV>(b, -beta, -beta + 1, depth - R, stack + 1));
			b.undo_null_move();

			(stack + 1)->isNullSearch = false;


			if (null_eval >= beta)
			{
				if (null_eval >= INF - stack->ply) null_eval = beta; // avoid returning unproven mates
				return beta; // fail hard
			}

			// the null move search failed low - which means we may be faced with threat ..
			// record the "to" square and record the move as a possible threat (bonus to those moves attack/evading to square)			
			if ((stack + 1)->bestmove != MOVE_NONE &&
				!b.is_quiet((stack + 1)->bestmove) &&
				null_eval < alpha &&
				null_eval > NINF + stack->ply)
			{
				stack->threat = (stack + 1)->bestmove;
				stack->threat_gain = null_eval - static_eval;
			}
		}

		// internal iterative deepening		
		if (ttm == MOVE_NONE &&
			depth >= (pv_node ? 6 : 8) &&
			(pv_node || static_eval >= beta) &&
			!stack->isNullSearch &&
			!split_node &&
			!b.in_check())
		{
			int iid = depth - 2 - (pv_node ? 0 : depth / 4);
			(stack + 1)->isNullSearch = true;
			int v = 0;
			if (pv_node) v = search<PV>(b, alpha, beta, iid, stack + 1);
			else v = search<NONPV>(b, alpha, beta, iid, stack + 1);
			(stack + 1)->isNullSearch = false;

			if (hashTable.fetch(key, e)) ttm = e.move;
		}


		BoardData pd;
		MoveSelect ms;
		if (split_node) ms = *worker->currSplitBlock->ms;
		else
		{
			MoveGenerator mvs(b, PSEUDO_LEGAL);

			// draw by repetition and stalemate -- TODO : detect draw by repetition .. currently broken.
			if (b.is_draw(mvs.size())) return DRAW;

			ms.load(mvs, b, ttm, statistics, stack->killer1, stack->killer2, lastmove, stack->threat, stack->threat_gain);
		}

		U16 move;
		while (ms.nextmove(*stack, move, split_node))
		{
			// TODO : check if this slows down the search ... 
			if (UCI_SIGNALS.stop) return DRAW;
			//else if (UCI_SIGNALS.timesUp) checkMoreTime(b, stack);


			if (split_node) split_point->split_mutex.unlock();


			if (move == MOVE_NONE || !b.is_legal(move))
			{
				if (split_node) split_point->split_mutex.lock();
				continue;
			}


			// check data for move (piece is needed for calls to b.is_dangerous)...
			int piece = b.piece_on(get_from(move));
			bool givesCheck = b.checks_king(move);

			// move extensions
			int extension = 0;
			if (b.in_check()) //|| get_movetype(move) == CAPTURE)// || b.is_dangerous(move,piece)) // check if not mate ?
			{
				extension = 1;
			}

			// move reductions
			int reduction = 0;

			if (moves_searched >= 5 &&
				depth >= 10 &&
				!b.in_check() &&
				!givesCheck &&
				move != ttm &&
				move != stack->killer1 &&
				move != stack->killer2 &&
				b.is_quiet(move))
			{
				reduction = 1;
				if (depth > 10 && !pv_node) reduction += 1;
			}

			// adjust search depth based on reduction/extensions
			int newdepth = depth + extension - reduction - 1;

			// futility pruning
			if (newdepth <= 1 &&
				move != ttm &&
				move != stack->killer1 &&
				move != stack->killer2 &&
				!b.in_check() &&
				!givesCheck &&
				b.is_quiet(move) &&
				//!pv_node &&
				eval + 450 < alpha &&
				eval > NINF + stack->ply &&
				!b.is_dangerous(move, piece))
			{
				continue;
			}


			// exchange pruning at shallow depths - same thing done in qsearch...		    			
			//if (depth <= 1 &&
			//	!b.in_check() &&
			//	!givesCheck &&
			//	move != ttm &&
			//	move != stack->killer1 &&
			//	move != stack->killer2 &&
			//	//!pv_node &&
			//	eval < alpha && 
			//	get_movetype(move) == CAPTURE &&
			//	b.see_move(move) <= 0) continue;

			bool pvMove = (moves_searched == 0 && pv_node);
			b.do_move(pd, move);

			stack->currmove = move;
			stack->givescheck = givesCheck;

			if (split_node) alpha = split_point->alpha;

			if (pvMove)
				eval = (newdepth <= 1 ? -qsearch<PV>(b, -beta, -alpha, newdepth, stack + 1, givesCheck) : -search<PV>(b, -beta, -alpha, newdepth, stack + 1));
			else
			{
				int R = newdepth;// -1;

				// entry point for pv-nodes to be passed to search as nonpv nodes
				if (move != ttm &&
					move != stack->killer1 &&
					move != stack->killer2 &&
					//eval < alpha &&
					static_eval < alpha &&
					!b.in_check() &&
					!givesCheck &&
					b.piece_on(get_from(move)) != PAWN  &&
					R > 8)
				{
					R -= R / 4;// +(alpha - eval) / 110);

					eval = (R <= 1 ? -qsearch<NONPV>(b, -alpha - 1, -alpha, R, stack + 1, givesCheck) : -search<NONPV>(b, -alpha - 1, -alpha, R, stack + 1));
				}
				else
				{
					if (move != ttm && move != stack->killer1 && (b.is_quiet(move) && !b.in_check() && !givesCheck))
					{
						R -= 1;
						// hedwig will find certain tactical sequences faster with this uncommented, but play is weaker overall (TODO: more testing..)
						if (static_eval < alpha && (statistics.score(move, b.whos_move()) < 0 || !pv_node || quiets_searched > 0)) R -= 1;
						//if (moves_searched >= 17 && static_eval < alpha) R -= 1;

					}

					// null window searches for pv & non-pv nodes.
					{
						//if (pv_node)
							eval = (R <= 1 ? -qsearch<PV>(b, -alpha - 1, -alpha, R, stack + 1, givesCheck) : -search<PV>(b, -alpha - 1, -alpha, R, stack + 1));
						//else
						//	eval = (R <= 1 ? -qsearch<NONPV>(b, -alpha - 1, -alpha, R, stack + 1, givesCheck) : -search<NONPV>(b, -alpha - 1, -alpha, R, stack + 1));
					}

				}
				// if eval raised alpha after null window search, research with full window, at full depth
				if ((eval > alpha && eval < beta))
				{
					eval = (newdepth <= 1 ? -qsearch<PV>(b, -beta, -alpha, newdepth, stack + 1, givesCheck) : -search<PV>(b, -beta, -alpha, newdepth, stack + 1));
				}
			}

			b.undo_move(move);



			if (split_node)
			{
				split_point->split_mutex.lock();
				alpha = split_point->alpha;
			}

			moves_searched++;

			if (UCI_SIGNALS.stop || (worker->found_cutoff() && split_node))
			{
				//printf("..found cutoff at thread-id(%d) (after search-call), returning 0\n", b.get_worker()->idx);
				return DRAW;
				//break;
			}
			//else if (UCI_SIGNALS.timesUp) checkMoreTime(b, stack);

			// update limits/besteval and bestmove
			if (eval >= beta)
			{
				if (split_node)
				{
					//printf("..found cutoff at thread-id(%d) normal search eval(%d), beta(%d), returning 0\n", b.get_worker()->idx, eval, beta );
					split_point->cut_occurred = true;
				}
				statistics.update(move, lastmove, stack, depth, b.whos_move(), quiets);
				hashTable.store(key, data, depth, BOUND_LOW, move, adjust_score(beta, stack->ply), static_eval, iter_depth);
				return beta;
			}

			if (eval > alpha)
			{
				if (split_node && eval > split_point->besteval)
				{
					//split_point->alpha = alpha;
					split_point->besteval = eval;
					split_point->bestmove = move;
				}
				stack->bestmove = move;
				alpha = eval;
				bestmove = move;

				// update the stored pv (used to force entries into the tt-table)
				update_pv(stack->pv, move, (stack + 1)->pv);
			}

			// move statistics - full search of quiet move
			if (get_movetype(move) == QUIET && quiets_searched < MAXDEPTH - 1 && move != bestmove)
			{
				quiets[quiets_searched++] = move;
				quiets[quiets_searched] = MOVE_NONE;
			}

			// idea is to flag moves which escape captures
			/*
			if (depth>=3 &&
			stack->currmove != MOVE_NONE && (stack-1)->currmove != MOVE_NONE &&
			stack->isNullSearch && moves_searched > 2 &&
			get_to((stack-1)->currmove) == get_from(stack->currmove) )//)
			{
			b.print();
			std::cout << UCI::move_to_string(stack->currmove) << " and " << UCI::move_to_string((stack-1)->currmove) << " reltated? " << std::endl;
			}
			*/


			// ybw split..
			if (depth >= min_split_depth + 10000
				&& !split_node
				&& stack->type == CUT
				&& Threads.size() >= 2
				&& (!worker->currSplitBlock
					|| !worker->currSplitBlock->allSlavesSearching)
				&& worker->nb_splits < SPLITS_PER_THREAD)
			{
				//printf("..splitting at thread-id(%d), depth(%d), alpha(%d), eval(%d), beta(%d), bestmove ",b.get_worker()->idx, depth, alpha, eval, beta);
				//std::cout << UCI::move_to_string(bestmove) << std::endl;
				if (!worker->split(b, &bestmove, stack, alpha, beta, &alpha, depth, &ms, stack->type == CUT))
				{
					dbg(Log::write(" .. split request failed\n"););
				}
				//printf("  after split : thread-id(%d), depth(%d), alpha(%d), eval(%d), beta(%d), bestmove ",b.get_worker()->idx,depth, alpha, eval, beta);
				//std::cout << UCI::move_to_string(bestmove) << std::endl;

				//printf("..found cutoff at thread-id(%d) (after split), returning 0\n", b.get_worker()->idx);
				if (UCI_SIGNALS.stop || worker->found_cutoff()) { return DRAW; }
				//printf("..found cutoff at thread-id(%d) (after split), returning 0\n", b.get_worker()->idx);

				// break here?
				if (alpha >= beta) { (stack + 1)->type = CUT;  return beta; }
			}

		} // end loop over mvs

		if (split_node) return split_point->besteval; //{ alpha =  split_point->besteval; bestmove = split_point->bestmove; } //return split_point->besteval;


		// mate detection
		//if (!moves_searched)
		//{
		//	if (b.in_check())
		//	{
		//		alpha = NINF + stack->ply;
		//	}
		//	else alpha = DRAW; // stalemate
		//}


		if (!split_node) // && !stack->isNullSearch && bestmove != MOVE_NONE)		
		{
			Bound ttb = BOUND_NONE;
			if (alpha <= aorig)
			{
				ttb = BOUND_HIGH;
			}
			else if (alpha >= beta)
			{
				ttb = BOUND_LOW;
			}
			else ttb = BOUND_EXACT;

			if (ttb != BOUND_NONE) hashTable.store(key, data, depth, ttb, bestmove, adjust_score(alpha, stack->ply), static_eval, iter_depth);
		}
		return alpha;
	}

	template<NodeType type>
	int qsearch(Board& b, int alpha, int beta, int depth, Node* stack, bool inCheck)
	{
		int eval = NINF;
		int ttval = NINF;
		int ttstatic_value = NINF;

		U64 key = 0ULL;
		U64 data = 0ULL;
		
		TableEntry e;
		bool pv_node = (type == ROOT || type == PV);
		
		U16 ttm = MOVE_NONE;
		U16 bestmove = MOVE_NONE;
		
		int aorig = alpha;
		bool split = (b.get_worker()->currSplitBlock != NULL);
		SplitBlock * split_point;
		if (split) split_point = b.get_worker()->currSplitBlock;

		
		stack->ply = (stack - 1)->ply++;
		U16 lastmove = (stack - 1)->currmove;
		int moves_searched = 0;
		

		// transposition table lookup at this node
		{
			data = b.data_key();
			key = b.pos_key();
			if (hashTable.fetch(key, e) &&
				e.depth >= depth)
			{
				ttm = e.move;
				ttstatic_value = e.static_value;
				if (e.bound == BOUND_EXACT && e.value > alpha && e.value < beta) return e.value;

				else if (e.bound == BOUND_LOW && !pv_node) return e.value;
				else if (e.bound == BOUND_HIGH && !pv_node) return e.value;
				
				if (alpha >= beta) return (int)beta;
				ttval = e.value;
			}
		}


		// stand pat lower bound -- tried using static_eval for the stand-pat value
		// however play was very weak..
		int stand_pat = (ttval = NINF ? Eval::evaluate(b) : ttval);
	
		// deprecated idea ... 
		//int stand_pat = (ttval > NINF ? ttval : ttstatic_value > NINF ? ttstatic_value : Eval::evaluate(b));
		//if (stand_pat == DRAW && ttstatic_value == DRAW) stand_pat = Eval::evaluate(b);

		// split search tt lookup
		//if (split)
		//{
		//	//split_point->split_mutex.lock();
		//	stand_pat = Eval::evaluate(b);
		//	//split_point->split_mutex.unlock();
		//}
		//else stand_pat = (ttval > NINF ? ttval : ttstatic_value > NINF ? ttstatic_value : Eval::evaluate(b));


		// standpat idea : but do not return during a check sequence
		if (stand_pat >= beta && !inCheck) 
			return beta;
		if (alpha < stand_pat && !inCheck)
			alpha = stand_pat;
		if (alpha >= beta && !inCheck) return beta;

		
		// get all the captures+checks/or evasions
		// NOTE : searching only checks or captures causes it to miss obv. "forcing moves" at higher depths!!
		//MoveGenerator mvs(b,inCheck);
		MoveGenerator mvs(b, CAPTURE); // play is a little stronger with this one

		// this return from search weakens play
		// note : draw detection happens in main search method
		//if (mvs.size() == 0 && !inCheck) return stand_pat;
		
		// mate detection
		if (inCheck && mvs.size() == 0) return NINF + stack->ply;

		//if (split) split_point->split_mutex.lock();
		MoveSelect ms;
		U16 killer = MOVE_NONE;
		ms.load(mvs, b, ttm, statistics, killer, killer, killer, killer, 0);
		//if (split) split_point->split_mutex.unlock();
		U16 move;
		Node dummy;

		// movegenerator generates only legal moves in qsearch (fyi)
		while (ms.nextmove(dummy, move, false))
		{
			if (move == MOVE_NONE) continue;

			// 0. prune quiet evasions -- leads to weaker play
			//if (inCheck &&
			//	b.is_quiet(move))
			//{
			//	stack->currmove = move;
			//	continue;
			//}

			//1. futility pruning
			//if (!inCheck &&
			//	!b.checks_king(move) &&
			//	!pv_node &&
			//	move != ttm &&
			//	b.piece_on(get_from(move)) != PAWN && 
			//	stand_pat + 100 < alpha)
			//{
			//	int to = get_to(move);
			//	int from = get_from(move);
			//	int increase = material.material_value((GamePhase)b.phase(), b.piece_on(to)) - material.material_value(b.phase(), b.piece_on(from));
			//	if (stand_pat + increase < alpha) continue;
			//}


			// prune captures which have see values < 0	  
			if (!inCheck &&
				!b.checks_king(move) &&
				!pv_node &&
				move != ttm &&
				eval <= alpha &&
				b.see_move(move) < 0)
				continue;

			BoardData pd;

			b.do_move(pd, move);

			bool givesCheck = b.gives_check(move);

			eval = -qsearch<type>(b, -beta, -alpha, depth - 1, stack + 1, givesCheck);

			b.undo_move(move);

			moves_searched++;

			if (eval >= beta)
			{
				hashTable.store(key, data, depth, BOUND_LOW, move, adjust_score(beta, stack->ply), stand_pat, iter_depth);
				return beta;
			}
			if (eval > alpha)
			{
				alpha = eval;
				bestmove = move;
				stack->currmove = move;
			}
		}

		//if (split) return alpha;

		//if (use_tt) //&& bestmove != MOVE_NONE)		
		{
			Bound ttb = BOUND_NONE;
			if (alpha <= aorig)
			{
				ttb = BOUND_HIGH;
			}
			else if (alpha >= beta)
			{
				ttb = BOUND_LOW;
			}
			else if (alpha > aorig && alpha < beta) ttb = BOUND_EXACT;

			//if (ttb != BOUND_NONE) ttable.store(key,data,depth,ttb,bestmove,alpha,NINF,iter_depth);
			if (ttb != BOUND_NONE) hashTable.store(key, data, depth, ttb, bestmove, adjust_score(alpha, stack->ply), stand_pat, iter_depth);
		}

		return alpha;
	}

	// from stockfish - standard method to store the pv-line (also see chess programming wiki)	
	void update_pv(U16* pv, U16& move, U16* child_pv)
	{
		for (*pv++ = move; *child_pv && *child_pv != MOVE_NONE;)
		{
			*pv++ = *child_pv++;
		}
		*pv = MOVE_NONE;
		//int j=0;
		//U16 tmp[MAXDEPTH];
		////std::memset(pv, 0, MAXDEPTH * sizeof(U16));

		//while(U16 m = (stack-j)->bestmove)
		//{
		//	if (m == MOVE_NONE) break;
		//	tmp[j++] = m;
		//}
		//
		//for (int i =0; i < j; ++i) pv[i] = tmp[j-i-1];

		//int k=0;
		//std::cout << std::endl;
		//while (U16 m = pv[k])
		//{
		//	if (m == MOVE_NONE) break;
		//	std::cout << SanSquares[get_from(m)] + SanSquares[get_to(m)] << " ";
		//	k++;
		//}
		//std::cout << std::endl;
		//std::cout << std::endl;
	}
	int adjust_score(int bestScore, int ply)
	{
		return (bestScore >= MATE_IN_MAXPLY ? bestScore = bestScore - ply :
			bestScore <= MATED_IN_MAXPLY ? bestScore + ply : bestScore);
	}
	void write_pv_to_tt(Board& b, U16* moves, int depth)
	{
		//Board copy = b;
		TableEntry e;
		int j = 0;
		BoardData pd[MAX_MOVES + 2], *st = pd;

		U16 m = MOVE_NONE;
		while ((m = moves[j++]) != MOVE_NONE)
		{
			if (m == MOVE_NONE) break;
			//std::cout << ".. try mv " << UCI::move_to_string(m) << std::endl;
			U64 pk = b.pos_key();
			U64 dk = b.data_key();
			// could consider storing even if tt-hit
			if (!hashTable.fetch(pk, e) || (hashTable.fetch(pk, e) && m != e.move))
			{
				// flag entries with exact bounds -- these are pv nodes from the search anyway
				hashTable.store(pk, dk, 0, BOUND_NONE, m, DRAW, NINF, iter_depth);
			}
			b.do_move(*st++, m);
			//b.print();
		}
		j--;
		// printf("nb mvs = %d\n",j);

		while (j) b.undo_move(moves[--j]);
		//b.print();
	}
	void uci_readout(Board& b, int depth)
	{
		int j = 0;
		U16 ttMoves[MAX_MOVES];
		TableEntry e;

		// check if e.move is actually legal here...possible tt move is not legal!
		// sometimes the tt move is actually illegal!!! need to fix that (especially for move ordering :-/)
		Board local_copy = b;

		do
		{
			U64 pk = local_copy.pos_key();

			if (hashTable.fetch(pk, e))
			{
				BoardData * pd = new BoardData(); // should go out of scope and be deleted (so no mem leak here.)

				if (e.move == MOVE_NONE) break;
				ttMoves[j] = e.move;
				if (j < 2) BestMoves[j] = e.move;
				std::string movestr = UCI::move_to_string(ttMoves[j]);
				std::cout << movestr << " ";
				local_copy.do_move(*pd, ttMoves[j++]);
			}
			else break;

		} while (e.move != MOVE_NONE && j < MAX_MOVES && j < int(depth));
		std::cout << "\n";

		j--;
		if (j > 0)
		{
			int k = j;
			RootMoves.clear();
			for (k = 0; k < j; ++k) RootMoves.push_back(ttMoves[k]);
		}
		while (j >= 0) local_copy.undo_move(ttMoves[j--]);

	}

	void checkMoreTime(Board& b, Node * stack)
	{
		//timer_thread->moretime_mutex.lock();
		timer_thread->adding_time = true;

		int j = 0;
		U16 move = MOVE_NONE;
		while (U16 m = (stack)->pv[j])
		{
			if (m == MOVE_NONE) break;
			move = m;
			j++;
		}

		//bool givesCheck = b.gives_check(move);
		//bool quiet = b.is_quiet(move);
		//bool dangerous = b.is_dangerous(move, b.piece_on(get_from(move)));

		int eval_change = 0;
		if (searchEvals.size() - 2 >= 0) eval_change = (searchEvals[searchEvals.size() - 1] - searchEvals[searchEvals.size() - 2]);

		bool moretime = (eval_change >= 50) || (eval_change <= -50 && searchEvals[searchEvals.size() - 1] > -20); // ((givesCheck || dangerous || !quiet) || (eval_change >= 33) || (eval_change <= -33 && searchEvals[searchEvals.size() - 1] > -20));

		if (moretime) nb_time_allocs++;

		// attempt to avoid case where score is oscillating and there is not much time left on the clock anyway
		if (moretime && eval_change > 100)
		{
			int timeleft = (b.whos_move() == WHITE ? timer_thread->search_limits->wtime : timer_thread->search_limits->btime);

			if (timeleft <= 2000 * 60) moretime = false;
			if (timeleft > 2000 * 60 && eval_change > 350) moretime = false; // should be ok to make the move
		}
		else if (moretime && eval_change < -100)
		{
			int timeleft = (b.whos_move() == WHITE ? timer_thread->search_limits->wtime : timer_thread->search_limits->btime);
			if (timeleft <= 2000 * 60) moretime = false;
			if (moretime && eval_change < -350) moretime = false; // we are probably lost here anyway...
		}

		if (moretime && nb_time_allocs > 3)
		{
			moretime = false;
			//std::cout << "...DBG allocated " << nb_time_allocs << " abort search here..." << std::endl;
		}
		//if (searchEvals.size() - 2 >= 0)
			//std::cout << "...DBG curr eval " << searchEvals[searchEvals.size() - 1] << " prev eval " << searchEvals[searchEvals.size() - 2] << " diff " << eval_change << " more time " << moretime << std::endl;


		UCI_SIGNALS.timesUp = !moretime;

		// signal the timer thread that we are finished checking if we need more time 
		timer_thread->adding_time = false;
		timer_thread->moretime_condition.signal();


	}
}
