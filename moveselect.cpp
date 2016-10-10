#include <algorithm>
#include <functional>

#include "moveselect.h"
#include "globals.h"
#include "evaluate.h"
#include "board.h"
#include "hashtable.h"
#include "threads.h"
#include "squares.h"
#include "search.h"
#include "uci.h"

struct {
	bool operator()(const MoveList& x, const MoveList& y)
	{
		return x.score > y.score; // nb. linux sorting needs > not >=
	}
} GreaterThan;

int piece_vals[5] = { PawnValueMG, KnightValueMG, BishopValueMG, RookValueMG, QueenValueMG };

void MoveStats::update(Board& b, U16& m, U16& last, Node* stack, int d, int eval, U16 * quiets)
{
	// called when eval >= beta in main search --> last move was a blunder (?)
	if (m == MOVE_NONE) return;
	int f = get_from(m);
	int t = get_to(m);
	int type = int((m & 0xf000) >> 12);
	int c = b.whos_move();
	//bool isQuiet = b.is_quiet(m);
	if (type == QUIET)
	{
		history[c][f][t] += pow(2, d);
		//printf("%d\n",history[c][f][t]);
	}

	// if we get here, eval >= beta, so last move is most likely a blunder, reduce the history score of it
	if (last != MOVE_NONE)
	{
		int type = int((last & 0xf000) >> 12);
		if (type == QUIET)
		{
			int f = get_from(last);
			int t = get_to(last);
			history[c = WHITE ? BLACK : WHITE][f][t] -= pow(2, d);
			countermoves[f][t] = m;
		}
	}

	if (quiets)
	{
		for (int j = 0; U16 mv = quiets[j]; ++j)
		{
			if (m == mv) continue;
			else if (mv == MOVE_NONE) break;
			int f = get_from(mv);
			int t = get_to(mv);
			history[c][f][t] -= pow(2, d);
		}
	}

	// update the stack killers
	if ((type == QUIET) && m != stack->killer[0])
	{
		stack->killer[1] = stack->killer[0];
		stack->killer[0] = m;
	}

	// mate killers
	if (eval >= MATE_IN_MAXPLY && m != stack->killer[2])
	{
		stack->killer[3] = stack->killer[2];
		stack->killer[2] = m;
	}
}

// dbg
void MoveSelect::print_list(Board& b)
{
	/*
	  if (ttmv)
		{
		  std::string s = UCI::move_to_string(ttmv);
		  std::cout << "currmove " << s << " (ttmove)" << std::endl;
		}
	  if (stack->killer[0])
		{
		  std::string s = UCI::move_to_string(stack->killer[0]);
		  std::cout << "currmove " << s << " (killer1)" << std::endl;
		}
	  if (stack->killer[1])
		{
		  std::string s = UCI::move_to_string(stack->killer[1]);
		  std::cout << "currmove " << s << " (killer2)" << std::endl;
		}
	*/
	printf("     !!DBG moveselect-sorting :: tomove(%s), type(%s), incheck(%s)\n", (b.whos_move() == WHITE ? "white" : "black"), (type == QUIET ? "quiet" : type == CAPTURE ? "capture" : "unknown"), b.in_check() ? "yes" : "no");
	if (captures)
	{
		printf("...captures...\n");
		MoveList * ml = captures;
		for (; ; ml++)
		{
			if (ml->m == MOVE_NONE) break;
			std::string s = UCI::move_to_string(ml->m);
			std::cout << "currmove " << s << " " << ml->score << std::endl;
		}
	}
	if (quiets)
	{
		printf("...quiets...\n");
		MoveList * ml = quiets;
		for (; ; ml++)
		{
			if (ml->m == MOVE_NONE) break;
			std::string s = UCI::move_to_string(ml->m);
			std::cout << "currmove " << s << " " << ml->score << std::endl;
		}
	}
	printf(".....end....\n\n");

}

void MoveSelect::load_and_sort(MoveGenerator& mvs, Board& b, U16& ttm, Node * stack, MoveType movetype)
{
	U16 killer1 = stack->killer[0];
	U16 killer2 = stack->killer[1];
	U16 mate1 = stack->killer[2];
	U16 mate2 = stack->killer[3];
	U16 lastmove = (stack - 1)->currmove;
	U16 threat = stack->threat;
	bool inCheck = b.in_check();

	for (; !mvs.end(); ++mvs)
	{
		U16 m = mvs.move();
		int from = int(m & 0x3f);
		int to = int((m & 0xfc0) >> 6);
		int mt = int((m & 0xf000) >> 12);
		int fp = b.piece_on(from);
		int tp = b.piece_on(to);
		bool checksKing = b.gives_check(m);

		if (m == ttm) continue;
		if (m == killer1 || m == killer2 || m == mate1 || m == mate2) continue;

		// build capture list -- evasions include quiet moves (fyi)
		if (movetype == CAPTURE &&
			(mt == CAPTURE || mt == EP || (mt <= PROMOTION_CAP && mt > PROMOTION) || (includeQsearchChecks() && checksKing)))

		{
			captures[c_sz].m = m;
			int score = 0;
			if0(mt == EP)
			{
				captures[c_sz++].score = score;
				continue;
			}
		//else if (piece_vals[tp] - piece_vals[fp] >= 0) score = piece_vals[tp] - piece_vals[fp]; // introduces tactical bugs
		else if (b.is_legal(m)) score = b.see_sign(m);
		else continue;

		// check bonus
		if ((Globals::SquareBB[from] & b.discovered_blockers(b.whos_move()) && b.is_dangerous(m, fp)))
		{
			score += 1;//piece_vals[b.piece_on(to)]; // almost always a good move
		}
		if ((Globals::SquareBB[from] & b.checkers()) && b.dangerous_check(m, false)) score += 1;

		// promotion bonus
		//if (mt > PROMOTION && mt <= PROMOTION_CAP) score += piece_vals[(type-4)];

		captures[c_sz++].score = score;
		}
		else if ((mt == QUIET || mt == CASTLE_KS || mt == CASTLE_QS || mt <= PROMOTION)) // build quiet list
		{
			quiets[q_sz].m = m;
			int score = statistics->score(m, b.whos_move());
			//printf("   ...looking at %s, stats(%d)\n", UCI::move_to_string(m).c_str(), statistics->score(m, b.whos_move()));
			// countermove bonus
			if (lastmove != MOVE_NONE &&
				m == statistics->countermoves[get_from(lastmove)][get_to(lastmove)])
				score += 10;

			// bonus for avoiding the capture from the threat move (from null search)
			if (threat != MOVE_NONE && get_to(threat) == from) score += 1; //piece_vals[b.piece_on(from)] / 2;

			// if previous bestmove attacks the from-sq, give a bonus for avoiding the capture/attack			
			if (to_sq(lastmove) == from)
			{
				score += 1;
			}

			// check bonus
			if ((Globals::SquareBB[from] & b.discovered_blockers(b.whos_move())) && b.piece_on(from) > PAWN)
			{
				score += 1; // keep small (many not dangerous moves satisfy criteria)			
			}
			if ((Globals::SquareBB[from] & b.checkers()) && b.is_dangerous(m, false)) score += 1;

			// promotion bonus
			//if (mt <= PROMOTION) score += 1;//piece_vals[type];

			// square score based ordering if score is unchanged.
			if (score <= (NINF - 1))
			{
				if (Globals::SquareBB[from] && b.checkers()) score += 1;
				//score += b.see_move(m);
				score += (square_score(b.whos_move(), fp, b.phase(), to) - square_score(b.whos_move(), fp, b.phase(), from))*0.1;
				//printf("      ...to(%d)-from(%d) :: adj(%d)\n", square_score(b.whos_move(), fp, b.phase(), to), square_score(b.whos_move(), fp, b.phase(), from), score);
			}
			//printf("   ...finalscore=%d, q_sz=%d\n",score, q_sz);
			quiets[q_sz++].score = score;// (score < NINF - 1 ? NINF - 1 : score);
		}
	}

	// insertion sort based on score
	stored_qsz = stored_csz = 0;
	if (q_sz > 0 && movetype == QUIET)
	{
		std::sort(quiets, quiets + q_sz, GreaterThan);
		stored_qsz = q_sz;
		q_sz = 0; // reset indices to 0, they index the quiet/capture arrays now     
	}
	else if (c_sz > 0 && movetype == CAPTURE)
	{
		std::sort(captures, captures + c_sz, GreaterThan);
		stored_csz = c_sz;
		c_sz = 0;
	}
	//print_list(b);
}

// note : scores are initialized to NINF-1
// so we want to sort the least negative of the scores to the front of the
// move list..
void MoveSelect::sort(MoveList * ml, int length)
{
	std::sort(ml, ml + length, GreaterThan);
}

bool MoveSelect::nextmove(Board &b, Node * stack, U16& ttm, U16& out, bool split)
{
	if (split)
	{
		// call nextmove from the thread data at this node.
		//U16 move;
		//bool do_next = b.get_worker()->currSplitBlock->ms->nextmove(b,out,false);//node->ms->nextmove(b,out,false);
		//node.sb->split_mutex.lock();
		//bool do_next = stack->sb->ms->nextmove(b, stack, ttm, move, false);
		//node.sb->split_mutex.unlock();
		//if (out == MOVE_NONE || !do_next) return false;
		//else { out = move; return true; }
		// dbg 
		//std::cout << std::endl;
		//std::cout << " ==================== thread- " << b.get_worker()->idx << " ===================== " << std::endl;
		//std::cout << " dbg all quiets " << std::endl;
		//int qsz=0;
		//MoveList * tmp_quiets = b.get_worker()->currSplitBlock->ms->get_quites(qsz);//node->ms->get_quites(qsz);
		//for (int j=0; j<qsz; ++j) std::cout << " " << SanSquares[get_from(tmp_quiets[j].m)] + SanSquares[get_to(tmp_quiets[j].m)];
		//std::cout << std::endl;

		//std::cout << " dbg all caps " << std::endl;
		//int csz=0;
		//MoveList * tmp_caps = b.get_worker()->currSplitBlock->ms->get_captures(csz);//node->ms->get_captures(csz);
		//for (int j=0; j<csz; ++j) std::cout << " " << SanSquares[get_from(tmp_caps[j].m)] + SanSquares[get_to(tmp_caps[j].m)];
		//std::cout << " thread-" << b.get_worker()->idx << " mv "<< SanSquares[get_from(out)] + SanSquares[get_to(out)] << std::endl;
		//std::cout << std::endl;
		return false;//do_next;
	}

	out = MOVE_NONE;
	if (phase >= End) return false;
	switch (phase)
	{
	case HashMove:
		if (type == QsearchCaptures && (b.is_qsearch_mv(ttm))) out = ttm;
		else if (type == MainSearch) out = ttm;
		phase++;
		break;

	case Killer1:
		if (type == MainSearch) out = stack->killer[0];
		phase++;
		return true;

	case Killer2:
		if (type == MainSearch) out = stack->killer[1];
		phase++;
		return true;

	case MateKiller1:
		out = stack->killer[2];
		phase++;
		return true;

	case MateKiller2:
		out = stack->killer[3];
		phase++;
		return true;

	case GoodCaptures:
		if (stored_csz == 0)
		{
			MoveGenerator mvs;
			if (type == MainSearch) mvs.generate_pseudo_legal(b, CAPTURE);
			else if (type == QsearchCaptures) mvs.generate_qsearch_mvs(b, CAPTURE, genChecks); // only generates checks if givesCheck == true
			load_and_sort(mvs, b, ttm, stack, CAPTURE);
		}
		if (captures[c_sz].score >= 0 && captures[c_sz].m != MOVE_NONE)
		{
			out = captures[c_sz].m; c_sz++; return true;
		}
		phase++;
		break;

	case BadCaptures:
		if (captures[c_sz].score < 0 && captures[c_sz].m != MOVE_NONE)
		{
			out = captures[c_sz].m; c_sz++; return true;
		}
		phase++;
		break;

	case Quiet:
		if (type == QsearchCaptures)
		{
			if (!b.in_check()) return false;
		}
		if (stored_qsz == 0)  // if we are in check and have not found an evasion, generate moves even in qsearch
		{
			MoveGenerator mvs; //mvs.generate_pseudo_legal(b, QUIET);
			if (type == MainSearch) mvs.generate_pseudo_legal(b, QUIET);
			else if (type == QsearchCaptures && b.in_check()) mvs.generate_qsearch_mvs(b, QUIET, genChecks); // hack to generate quiet evasions (only when in check)
			load_and_sort(mvs, b, ttm, stack, QUIET);
		}
		if (stored_qsz > 0 && quiets[q_sz].m != MOVE_NONE)
		{
			out = quiets[q_sz].m; q_sz++; return true;
		}
		phase++;
		return false;
	}
	return true;
}


