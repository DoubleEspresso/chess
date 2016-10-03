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
	int hash_hits = 0;

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
	void ReadoutRootMoves(int depth);
};

namespace Search
{
	void run(Board& b, int dpth, bool pondering)
	{
	  // nb: perf testing on linux reveals options[] lookup to be very slow
	  // best to avoid queries to options[] during search/evaluation
		// check opening table if using opening book
	  /*
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
	  */
		// init search params
		int alpha = NINF;
		int beta = INF;
		int eval = NINF;

		// search nodes init
		Node stack[128 + 4];
		std::memset(stack, 0, (128 + 4) * sizeof(Node));

		// clear search data
		iter_depth = 1;

		// start the timer thread now
		timer_thread->searching = true;
		timer_thread->elapsed = 0;
		timer_thread->sleep_condition.signal();

		// main entry point for the fail-hard alpha-beta search
		// the main iterative deepening loop
		for (int depth = 1; depth <= dpth; depth += 1)
		{
			if (UCI_SIGNALS.stop) break; hash_hits = 0;
			//else if (UCI_SIGNALS.timesUp) checkMoreTime(b, stack + 2);
			statistics.clear(); // move ordering of quiet moves
			(stack + 2)->ply = (stack + 1)->ply = (stack)->ply = 0;
			eval = search<ROOT>(b, alpha, beta, depth, stack + 2);
			iter_depth++;

			if (timer_thread->elapsed - last_time_ms >= 3000) ReadoutRootMoves(depth);

			if (!UCI_SIGNALS.stop)
			{
				print_pv_info(b, depth, eval, (stack + 2)->pv);
				last_time_ms = timer_thread->elapsed;
				RootMoves.clear();
			}
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
		return Globals::SearchReductions[(int)pv_node][(int)improving][MAX(0, MIN(d, MAXDEPTH - 1))][MAX(0, MIN(mc, MAXDEPTH - 1))];
	}

	bool RootsContain(U16& root_mv)
	{
		return std::find(RootMoves.begin(), RootMoves.end(), root_mv) != RootMoves.end();
	}

	template<NodeType type>
	int search(Board& b, int alpha, int beta, int depth, Node* stack)
	{
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

		// update stack info
		stack->ply = (stack - 1)->ply++;
		stack->currmove = stack->bestmove = (stack + 2)->killer[0] = (stack + 2)->killer[1] = (stack + 2)->killer[2] = (stack + 2)->killer[3] = MOVE_NONE;
		stack->givescheck = false;

		// clear pv
		for (int j = 0; j < MAX_MOVES; ++j) stack->pv[j] = MOVE_NONE;

		// reductions array specific
		bool improving = false;

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
		int mate_dist = stack->ply;// iter_depth - depth + 1;

		//if (UCI_SIGNALS.stop) return DRAW;

		// 1. -- mate distance pruning    
		int mate_val = INF - mate_dist;
		beta = MIN(mate_val, beta);
		if (alpha >= mate_val)
		{
			//printf("!!DBG (search-mate) mate distance pruning :: depth(%d) alpha(%d),mate_val(%d),beta(%d), bm=%s\n", depth, alpha, mate_val, beta, UCI::move_to_string(stack->bestmove).c_str());
			return mate_val;
		}

		int mated_val = NINF + mate_dist;
		alpha = MAX(mated_val, alpha);
		if (beta <= mated_val)
		{
			//printf("!!DBG (search-mated) mate distance pruning :: depth(%d) alpha(%d),mated_val(%d),beta(%d), bm=%s\n", depth, alpha, mated_val, beta, UCI::move_to_string(stack->bestmove).c_str());
			return mated_val;
		}

		// 2. -- ttable lookup 
		data = b.data_key();
		key = b.pos_key();
		if (hashTable.fetch(key, e) && e.Depth() >= depth)
		{
			ttm = e.move;
			//ttstatic_value = e.static_value;	
			ttvalue = e.value;
			++hash_hits;

			if (pv_node)
				if (e.bound == BOUND_EXACT && e.value > alpha && e.value < beta)
				{
					stack->currmove = stack->bestmove = e.move;
					return e.value;
				}
				else if (e.bound == BOUND_LOW && e.value >= beta)
				{
					if (ttm != MOVE_NONE) statistics.update(b, ttm, lastmove, stack, depth, eval, quiets);
					return e.value;
				}
				else if (e.bound == BOUND_HIGH  && e.value <= alpha) return  e.value;
		}

		// 3. -- static evaluation of position    
		//int static_eval = Eval::evaluate(b);
		int static_eval = (ttvalue > NINF ? ttvalue : Eval::evaluate(b));
		//int static_eval = (ttvalue > NINF ? ttvalue : ttstatic_value > NINF ? ttstatic_value : Eval::evaluate(b));

		// 4. -- drop into qsearch if we are losing
		if (depth <= 4 &&
			!pv_node && 
			ttm == MOVE_NONE &&
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
				return ralpha;
			}
		}

		// 5. -- futility pruning
		if (depth <= 6 &&
			!pv_node &&
			!b.in_check() &&
			!stack->isNullSearch &&
			static_eval - 200 * depth >= beta &&
			beta < INF - mate_dist &&
			b.non_pawn_material(b.whos_move()))
		{
			return beta; // fail hard
		}

		// 6. -- null move search    
		if (!pv_node &&
			depth >= 2 &&
			!stack->isNullSearch &&
			static_eval >= beta &&
			!b.in_check() &&
			b.non_pawn_material(b.whos_move()))
		{
			int R = (depth >= 8 ? depth / 2 : 2);
			BoardData pd;

			(stack + 1)->isNullSearch = true;
			b.do_null_move(pd);
			int null_eval = (depth - R <= 1 ? -qsearch<NONPV>(b, -beta, -beta + 1, 0, stack + 1, false) : -search<NONPV>(b, -beta, -beta + 1, depth - R, stack + 1));
			b.undo_null_move();
			(stack + 1)->isNullSearch = false;

			if (null_eval >= beta)
			{
				return beta;
			}
			// the null move search failed low - which means we may be faced with threat ..
			// record the "to" square and record the move as a possible threat (bonus to those moves attack/evading to square)			
			if ((stack + 1)->currmove != MOVE_NONE &&
				//!b.is_quiet((stack + 1)->currmove) &&
				//null_eval < beta &&
				null_eval > NINF + mate_dist)
			{
				stack->threat = (stack + 1)->currmove;
				stack->threat_gain = null_eval - static_eval;
			}
		}

		// 7. -- probcut from stockfish
		if (!pv_node &&
			depth >= 6 &&
			!b.in_check() &&
			!stack->isNullSearch &&
			beta < MATE_IN_MAXPLY &&
			movetype((stack - 1)->currmove) == CAPTURE)
		{
			BoardData pd;
			MoveSelect ms(statistics, QsearchCaptures, false); // no checks just captures
			U16 move;
			int rbeta = beta + 620;
			int rdepth = depth - 2;
			while (ms.nextmove(b, stack, ttm, move, false))
			{
				if (!b.is_legal(move)) continue;
				b.do_move(pd, move);
				stack->currmove = move;
				eval = -search<NONPV>(b, -rbeta, -rbeta + 1, rdepth, stack + 1);
				b.undo_move(move);
				if (eval >= rbeta)
				{
					return beta;
				}
			}
		}

		// 7. -- internal iterative deepening
		if (ttm == MOVE_NONE &&
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
			if (hashTable.fetch(key, e))
			{
				ttm = e.move;
			}
		}

		// 6. -- moves search
		BoardData pd;
		MoveSelect ms(statistics, MainSearch);
		U16 move;
		int pruned = 0; moves_searched = 0;
		//if (ttm == MOVE_NONE) ttm = (stack-2)->pv[iter_depth-depth];

		improving = static_eval - (stack - 2)->static_eval >= 0 || static_eval == 0 ||
			(stack - 2)->static_eval == 0;

		//printf("!!DBG (search) start moves loop :: depth(%d) alpha(%d),eval(%d),beta(%d), bm=%s\n", depth, alpha, eval, beta, UCI::move_to_string(stack->bestmove).c_str());

		while (ms.nextmove(b, stack, ttm, move, false))
		{
			if (UCI_SIGNALS.stop) { return DRAW; }

			if (!b.is_legal(move))
			{
				++pruned;
				continue;
			}

			if (type == ROOT && !RootsContain(move)) RootMoves.push_back(move);

			// piece and move data
			int piece = b.piece_on(get_from(move));
			bool givesCheck = b.gives_check(move); stack->givescheck = givesCheck;
			bool inCheck = b.in_check();
			bool isQuiet = b.is_quiet(move);
			bool pvMove = (moves_searched == 0 && pv_node);

			// extension/reductions
			int extension = 0; int reduction = 1; // always reduce current depth by 1
			if (givesCheck) extension += 1;

			if (depth >= 8 &&
				!inCheck &&
				!givesCheck &&
				//isQuiet &&
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
				move != ttm &&
				move != stack->killer[0] &&
				move != stack->killer[1] &&
				!inCheck &&
				!givesCheck &&
				isQuiet &&
				!pv_node &&
				eval + 650 < alpha &&
				eval > NINF + mate_dist)
			{
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
				move != stack->killer[0] &&
				move != stack->killer[1] &&
				!givesCheck &&
				!inCheck &&
				isQuiet &&
				!b.pawn_on_7(b.whos_move()) &&
				quiets_searched > 3)
				//depth > (pv_node ? 6 : 4))
			{
				int R = Reduction(pv_node, improving, newdepth, moves_searched) / 2;
				int v = statistics.history[b.whos_move()][get_from(move)][get_to(move)];
				if (v <= (NINF - 1)) R += 1;
				int LMR = newdepth - R;
				eval = (LMR <= 1 ? -qsearch<NONPV>(b, -alpha - 1, -alpha, 0, stack + 1, givesCheck) : -search<NONPV>(b, -alpha - 1, -alpha, LMR, stack + 1));
				if (eval > alpha) fulldepthSearch = true;
			}
			else fulldepthSearch = !pvMove;

			if (fulldepthSearch)
			{
				//printf("!!DBG fulldepthSearch(true) move(%s) newdepth(%d), alpha(%d) eval(%d) beta(%d), nodes(%d), msnodes(%d), qsnodes(%d)\n", (b.whos_move() == WHITE ? "white" : "black"), newdepth, alpha, eval, beta, b.get_nodes_searched(), b.MSnodes(), b.QSnodes());
				eval = (newdepth <= 1 ? -qsearch<NONPV>(b, -alpha - 1, -alpha, 0, stack + 1, givesCheck) : -search<NONPV>(b, -alpha - 1, -alpha, newdepth, stack + 1));
			}

			if (pvMove || eval > alpha)
			{
				//printf("!!DBG pvSearch(true) move(%s) newdepth(%d), alpha(%d) eval(%d) beta(%d), nodes(%d), msnodes(%d), qsnodes(%d)\n", (b.whos_move() == WHITE ? "white" : "black"), newdepth, alpha, eval, beta, b.get_nodes_searched(), b.MSnodes(), b.QSnodes());

				eval = (newdepth <= 1 ? -qsearch<PV>(b, -beta, -alpha, 0, stack + 1, givesCheck) : -search<PV>(b, -beta, -alpha, newdepth, stack + 1));
			}

			b.undo_move(move);
			moves_searched++;
			if (UCI_SIGNALS.stop) return DRAW;

			// record move scores/evals
			if (eval >= beta)
			{
				//printf("!!DBG (search) beta cut :: depth(%d) alpha(%d),eval(%d),beta(%d), bm=%s\n", depth, alpha, eval, beta, UCI::move_to_string(move).c_str());

				if (isQuiet)
				{
					if (quiets_searched < MAXDEPTH - 1)
					{
						quiets[quiets_searched++] = move;
						quiets[quiets_searched] = MOVE_NONE;
					}
					statistics.update(b, move, lastmove, stack, depth, adjust_score(eval, mate_dist), quiets);
				}
				hashTable.store(key, data, depth, BOUND_LOW, move, adjust_score(beta, mate_dist), static_eval, pv_node);
				return beta;
			}
			if (eval > alpha)
			{
				//printf("!!DBG (search, eval>alpha) :: depth(%d) alpha(%d),eval(%d),beta(%d), bm=%s\n", depth, alpha, eval, beta, UCI::move_to_string(move).c_str());
				stack->bestmove = move;
				alpha = eval;
				bestmove = move;
				update_pv(stack->pv, move, (stack + 1)->pv);
			}
		}

		if (b.in_check() && !moves_searched)
		{
			//printf("!!DBG found mate (search) :: mate_dist(%d) alpha(%d),eval(%d),beta(%d), bm=%s\n", mate_dist, alpha, eval, beta, UCI::move_to_string(stack->bestmove).c_str());
			return NINF + mate_dist;
		}
		else if (b.is_draw(moves_searched)) return DRAW;

		// update gui 
		//if (timer_thread->elapsed - last_time_ms >= 3000)
		//{
		//	pv_from_tt(ROOT_BOARD, eval, iter_depth);
		//	last_time_ms = timer_thread->elapsed;
		//}

		Bound ttb = alpha <= aorig ? BOUND_HIGH : alpha >= beta ? BOUND_LOW : BOUND_EXACT;
		hashTable.store(key, data, depth, ttb, bestmove, adjust_score(alpha, mate_dist), static_eval, pv_node);

		return alpha;
	} // end search

	template<NodeType type>
	int qsearch(Board& b, int alpha, int beta, int depth, Node* stack, bool inCheck)
	{
 		if (b.is_repition_draw()) return DRAW;
		int eval = NINF;
		int ttval = NINF;
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
		int mate_dist = stack->ply;// iter_depth - depth + 1;
		//bool split = (b.get_worker()->currSplitBlock != NULL);
		//SplitBlock * split_point;
		//if (split) split_point = b.get_worker()->currSplitBlock;

		//U16 lastmove = (stack - 1)->currmove;
		bool genChecks = (stack - 2)->givescheck && depth > -3;
		int qsDepth = (genChecks ? -1 : 0);

		// transposition table lookup    
		data = b.data_key();
		key = b.pos_key();
		if (hashTable.fetch(key, e) && e.Depth() >= qsDepth)
		{
			++hash_hits;
			ttm = e.move;
			//ttstatic_value = e.static_value;
			ttval = e.value;
			if (pv_node)
				if (e.bound == BOUND_EXACT && e.value > alpha && e.value < beta) return e.value;
				else if (e.bound == BOUND_LOW && e.value >= beta) return  e.value;
				else if (e.bound == BOUND_HIGH && e.value <= alpha) return  e.value;
		}

		// stand pat lower bound -- tried using static_eval for the stand-pat value, play was weak
		//int stand_pat = (ttval > NINF && ttval > alpha && ttval < beta ? ttval : Eval::evaluate(b));
		int stand_pat= Eval::evaluate(b);    

		if (stand_pat >= beta && !inCheck) return beta;
		// delta pruning                 		
		if (stand_pat < alpha - 950 && !inCheck)
		{
			return alpha;
		}
		if (alpha < stand_pat && !inCheck) alpha = stand_pat;
		if (alpha >= beta && !inCheck) return beta;

		MoveSelect ms(statistics, QsearchCaptures, genChecks);
		int moves_searched = 0, pruned = 0;
		U16 move;

		while (ms.nextmove(b, stack, ttm, move, false))
		{
			if (!b.is_legal(move))
			{
				++pruned;
				continue;
			}

			// move data
			int piece = b.piece_on(get_from(move));
			bool checksKing = b.gives_check(move); stack->givescheck = checksKing;
			bool isQuiet = b.is_quiet(move); // evasions
			//bool discoveredBlocker = (Globals::SquareBB[get_from(move)] && b.discovered_blockers(b.whos_move()));

			// futility pruning --continue if we are winning	
			if (!checksKing &&
				!inCheck &&
				move != ttm &&
				!pv_node &&
				eval + material.material_value(b.piece_on(get_to(move)), END_GAME) >= beta)
			{
				eval += material.material_value(b.piece_on(get_to(move)), END_GAME);
				++pruned;
				continue;
			}

			// prune captures which have see values <= 0	
			if (!inCheck
				&& !pv_node
				&& !checksKing
				&& move != ttm
				&& b.see_move(move) < 0)
			{
				++pruned;
				continue;
			}

			BoardData pd;
			b.do_move(pd, move, true);
			eval = -qsearch<type>(b, -beta, -alpha, depth - 1, stack + 1, checksKing);
			b.undo_move(move);
			moves_searched++;

			if (eval >= beta)
			{
				//printf("!!DBG (qsearch) beta cut :: depth(%d) alpha(%d),eval(%d),beta(%d), bm=%s\n", depth, alpha, eval, beta, UCI::move_to_string(move).c_str());

				hashTable.store(key, data, qsDepth, BOUND_LOW, move, adjust_score(beta, mate_dist), stand_pat, pv_node);
				return beta;
			}
			if (eval > alpha)
			{
				//printf("!!DBG (qsearch) pv-node :: depth(%d) alpha(%d),eval(%d),beta(%d), bm=%s\n", depth, alpha, eval, beta, UCI::move_to_string(move).c_str());

				alpha = eval;
				bestmove = move;
				stack->currmove = move;
				update_pv(stack->pv, move, (stack + 1)->pv);
			}
		}

		// detect terminal node
		if (!moves_searched && b.in_check()) return NINF + mate_dist;
 		else if (b.is_repition_draw()) return DRAW;

		Bound ttb = alpha <= aorig ? BOUND_HIGH : alpha >= beta ? BOUND_LOW : BOUND_EXACT;
		hashTable.store(key, data, qsDepth, ttb, bestmove, adjust_score(alpha, mate_dist), stand_pat, pv_node);

		return alpha;
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
			BoardData * pd = new BoardData();
			//if (j < 2) BestMoves[j] = e.move;
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

	void ReadoutRootMoves(int depth)
	{
		for (int j = 0; j < RootMoves.size(); ++j)
		{
			printf("info depth %d currmove %s currmovenumber %d\n", depth, UCI::move_to_string(RootMoves[j]).c_str(), j + 1);
		}
	}

	void print_pv_info(Board& b, int depth, int eval, U16 * pv)
	{
		// mate scores ..
		int mate_val = (eval >= MATE_IN_MAXPLY ? (INF - eval + 1 )/2 : 0);
		int mated_val = (eval <= MATED_IN_MAXPLY ? (NINF - eval - 1 ) / 2 : 0);

		std::string pv_str = "";
		int j = 0;
		while (U16 m = pv[j])
		{
			if (m == MOVE_NONE) break;
			pv_str += UCI::move_to_string(m) + " ";
			if (j < 2) BestMoves[j] = m;//(!pondering ? BestMoves[j] = m : PonderMoves[j] = m);
			j++;
		}
		int elapsed_ms = (int)(timer_thread->elapsed <= 0 ? 1 : timer_thread->elapsed);
		printf("info score %s %d depth %d seldepth %d nodes %d qsnodes %d msnodes %d tbhits %d time %d nps %d pv ",
			(mate_val != 0 || mated_val != 0 ? "mate" : "cp"),
			(mate_val != 0 ? mate_val : mated_val != 0 ? mated_val : eval),
			depth,
			j,
			b.get_nodes_searched(),
			b.QSnodes(),
			b.MSnodes(),
			hash_hits,
			elapsed_ms, (int)  ( (float) 1000 * ((float) b.get_nodes_searched() / (float)elapsed_ms)));
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
