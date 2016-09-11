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


// sorting using std::sort()
struct {
	bool operator()(const MoveList& x, const MoveList& y)
	{
		return x.score > y.score;
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
}

// dbg print movelist -- deprecated
void MoveSelect::print_list()
{
	printf("\n.....start....\n");
	/*
	if (ttmv)
	  {
		std::string s = UCI::move_to_string(ttmv);
		std::cout << "currmove " << s << " (ttmove)" << std::endl;
	  }
	if (stack->killer1)
	  {
		std::string s = UCI::move_to_string(stack->killer1);
		std::cout << "currmove " << s << " (killer1)" << std::endl;
	  }
	if (stack->killer2)
	  {
		std::string s = UCI::move_to_string(stack->killer2);
		std::cout << "currmove " << s << " (killer2)" << std::endl;
	  }
	*/
	printf(".....captures....\n");
	if (captures)
	{
		for (int j = 0; U16 mv = captures[j].m; ++j)
		{
			std::string s = UCI::move_to_string(mv);
			std::cout << "currmove " << s << " " << captures[j].score << std::endl;
		}
	}
	printf(".....quiets....\n");
	if (quiets)
	{
		for (int j = 0; U16 mv = quiets[j].m; ++j)
		{
			std::string s = UCI::move_to_string(mv);
			std::cout << "currmove " << s << " " << quiets[j].score << std::endl;
		}
	}
	printf(".....end....\n\n");

}

void MoveSelect::load_and_sort(MoveGenerator& mvs, Board& b, U16& ttm, Node * stack, MoveType movetype)
{
	U16 killer1 = stack->killer[0];
	U16 killer2 = stack->killer[1];
	//U16 killer3 = stack->killer[2];  
	//U16 killer4 = stack->killer[3];  
	U16 lastmove = (stack - 1)->currmove;
	bool inCheck = b.in_check();

	for (; !mvs.end(); ++mvs)
	{
		U16 m = mvs.move();
		int from = int(m & 0x3f);
		int to = int((m & 0xfc0) >> 6);
		int mt = int((m & 0xf000) >> 12);
		int p = b.piece_on(to);

		if (m == ttm) continue;
		if (m == killer1 || m == killer2 ) continue; //|| m == killer3 || m == killer4

		// build capture list -- evasions include quiet moves (fyi)
		if (movetype == CAPTURE &&
			(mt == CAPTURE || mt == EP || (mt <= PROMOTION_CAP && mt > PROMOTION)))
		{
			captures[c_sz].m = m;
			int score = 0;
			if0(mt == EP)
			{
				captures[c_sz++].score = score;
				continue;
			}
			score = piece_vals[b.piece_on(to)] - piece_vals[b.piece_on(from)];
			// TODO :: speed this up ... ?
			if (score < 0 && b.is_legal(m)) score = b.see_move(m);

			// check bonus
			if ((Globals::SquareBB[from] & b.discovered_blockers(b.whos_move()) && b.is_dangerous(m, b.piece_on(from)))) 
			  {
			    score += 125; // almost always a good move
			  }
			//if ((Globals::SquareBB[from] & b.checkers()) && b.is_dangerous(m, b.piece_on(from))) score += 25;

			// promotion bonus
			//if (mt > PROMOTION && mt <= PROMOTION_CAP) score += piece_vals[(type-4)];

			captures[c_sz++].score = score;
		}
		else if ((mt == QUIET || mt == CASTLE_KS || mt == CASTLE_QS || mt <= PROMOTION)) // build quiet list
		{
			quiets[q_sz].m = m;
			int score = statistics->score(m, b.whos_move());

			// countermove bonus
			if (lastmove != MOVE_NONE &&
				m == statistics->countermoves[get_from(lastmove)][get_to(lastmove)])
				score += 25;

			// check bonus
			if ((Globals::SquareBB[from] & b.discovered_blockers(b.whos_move())) && b.piece_on(from) > PAWN) 
			{
				score += 25; // keep small (many not dangerous moves satisfy criteria)			
			}
			//if ((Globals::SquareBB[from] & b.checkers()) && b.is_dangerous(m, b.piece_on(from))) score += 25;

			// promotion bonus
			//if (mt <= PROMOTION) score += piece_vals[type];	  

			quiets[q_sz++].score = score;
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
	if (select_phase >= PHASE_END) return false;
	switch (select_phase)
	{
	case PHASE_TT:
		if (type == QSEARCH && b.is_qsearch_mv(ttm)) out = ttm;
		else if (type == MAIN) out = ttm;
		select_phase++;
		break;

	case PHASE_KILLER1:
		if (type == MAIN) out = stack->killer[0];
		select_phase++;
		return true;

	case PHASE_KILLER2:
		if (type == MAIN) out = stack->killer[1];
		select_phase++;
		return true;

	case PHASE_KILLER3:
		//out = stack->killer[2];
		select_phase++;
		return true;

	case PHASE_KILLER4:
		//out = stack->killer[3];
		select_phase++;
		return true;

	case PHASE_CAPTURE_GOOD:
		if (stored_csz == 0)
		{
			MoveGenerator mvs; //mvs.generate_pseudo_legal(b, CAPTURE);
			if (type == MAIN) mvs.generate_pseudo_legal(b, CAPTURE);
			else if (type == QSEARCH) mvs.generate_qsearch_mvs(b, CAPTURE, givesCheck); // only generates checks if givesCheck == true
			load_and_sort(mvs, b, ttm, stack, CAPTURE);
		}
		if (captures[c_sz].score >= 0 && captures[c_sz].m != MOVE_NONE)
		{
			out = captures[c_sz].m; c_sz++; return true;
		}
		select_phase++;
		break;

	case PHASE_CAPTURE_BAD:
		if (captures[c_sz].score < 0 && captures[c_sz].m != MOVE_NONE)
		{
			out = captures[c_sz].m; c_sz++; return true;
		}
		select_phase++;
		break;

	case PHASE_QUIET:
		if (type == QSEARCH)
		{
			if (!b.in_check()) return false;
		}
		if (stored_qsz == 0)  // if we are in check and have not found an evasion, generate moves even in qsearch
		{
			MoveGenerator mvs; //mvs.generate_pseudo_legal(b, QUIET);
			if (type == MAIN) mvs.generate_pseudo_legal(b, QUIET);
			else if (type == QSEARCH) mvs.generate_qsearch_mvs(b, QUIET, givesCheck); // only generates checks if givesCheck == true
			load_and_sort(mvs, b, ttm, stack, QUIET);
		}
		if (stored_qsz > 0 && quiets[q_sz].m != MOVE_NONE)
		{
			out = quiets[q_sz].m; q_sz++; return true;
		}
		select_phase++;
		return false;
	}
	return true;
}


