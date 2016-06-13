#include <algorithm>
#include <functional>

#include "moveselect.h"
#include "evaluate.h"
#include "board.h"
#include "hashtable.h"
#include "threads.h"
#include "squares.h"
#include "search.h"
#include "uci.h"

// sorting using std::sort()
struct {
  bool operator()(const MoveList& x,const MoveList& y)
  {
    return x.score > y.score;
  }
} GreaterThan;

int piece_vals[5] = { PawnValueMG, KnightValueMG, BishopValueMG, RookValueMG, QueenValueMG };

void MoveStats::update(U16& m, U16& last, Node* stack, int d, int c, U16 * quiets)
{
  // called when eval >= beta in main search --> last move was a blunder (?)
  if (m == MOVE_NONE) return;
  int f = get_from(m);
  int t = get_to(m);
  int type = int((m & 0xf000) >> 12);
  if (type == QUIET) history[c][f][t] += pow(2, d);

  // if we get here, eval >= beta, so last move is most likely a blunder, reduce the history score of it
  if (last != MOVE_NONE)
    {
      int f = get_from(last);
      int t = get_to(last);
      int type = int((last & 0xf000) >> 12);
      if (type == QUIET) history[c = WHITE ? BLACK : WHITE][f][t] -= pow(2, d); 
    }
  // reduce all other quiets -- do not uncomment -- see : position fen 1rb2rk1/5ppp/p2p4/2pP4/B3Q3/3P4/PqP2P1P/R4R1K w - - 2 19   
  //if (quiets)
  //{
	 // for (int j = 0; U16 mv = quiets[j]; ++j)
	 // {
		//  if (m == mv) continue;
		//  else if (mv == MOVE_NONE) break;
		//  int f = get_from(mv);
		//  int t = get_to(mv);
		//  history[c][f][t] -= pow(2, d);
	 // }
  //}

  // update the stack killers
  if (type == QUIET && m != stack->killer1) { stack->killer2 = stack->killer1; stack->killer1 = m; }
}

// dbg print movelist -- deprecated
void MoveSelect::print_list()
{
  printf("\n.....start....\n");
  if (ttmv)
    {
      std::string s = UCI::move_to_string(ttmv);
      std::cout << "currmove " << s << " (ttmove)" << std::endl;
    }
  if (killers[0])
    {
      std::string s = UCI::move_to_string(killers[0]);
      std::cout << "currmove " << s << " (killer-1)" << std::endl;
    }
  if (killers[1])
    {
      std::string s = UCI::move_to_string(killers[1]);
      std::cout << "currmove " << s << " (killer-2)" << std::endl;
    }
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

void MoveSelect::load(MoveGenerator& mvs, Board& b, U16 tt_mv, MoveStats& stats, U16& killer1, U16& killer2, U16& lastmove, U16& threat, int threatgain)
{
  statistics = &stats;
  
  for (; !mvs.end(); ++mvs)
    {
      U16 m = mvs.move();
      int from = int(m & 0x3f);
      int to = int((m & 0xfc0) >> 6);
      int mt = int((m & 0xf000) >> 12);
      int p = b.piece_on(from);
      int c = b.whos_move();
      
      int last_to = int((lastmove & 0xfc0) >> 6);
      int last_from = int(lastmove & 0x3f);
      
      if (m == killer1 && m != tt_mv) { killers[0] = m; continue; }
      else if (m == killer2 && m != tt_mv) { killers[1] = m; continue; }
      else if ((mt == CAPTURE || mt == PROMOTION_CAP || mt == EP) && m != tt_mv)
	{
	  captures[c_sz].m = m;
	  int score = 0;
	  // catch ep captures
	  if0(mt == EP)
	    {
	      captures[c_sz++].score = score;
	      continue;
	    }
	  
	  score = piece_vals[b.piece_on(to)] - piece_vals[b.piece_on(from)];
	  //score = b.see_move(m);
	  if (score <= 0) score = b.see_move(m);
	  if (score == 0 && b.checks_king(m) && b.is_dangerous(m, p)) score += 1;// piece_vals[b.piece_on(from)];
	  
	  // the threat move from null-refutation, bonus if we capture the threatening piece
	  // was only used if score == 0
	  //if (score == 0 && threat && to == get_from(threat)) score += piece_vals[b.piece_on(to)] / 2;// - vals_mg[b.piece_on(from)];
	  captures[c_sz++].score = score;
	  
	}
      else if ((mt == QUIET || mt == CASTLE_KS ||
		mt == CASTLE_QS || mt == PROMOTION || mt == EVASION) && m != tt_mv)
	{
	  quiets[q_sz].m = m;
	  int score = statistics->score(m, b.whos_move());
	  //printf("...initial score = %d ", score);

	  // gives bonus for evading being captured
	  if (b.attackers_of(from) & U64(last_to) )//&& get_to(threat) == get_from(m))//(b.attackers_of(from) & U64(get_to(threat))))//(last_to == from)
	    {
	      int diff = (piece_vals[b.piece_on(from)] - piece_vals[b.piece_on(get_to(threat))]);
	      //b.print();
	      //printf("..lastmove = %s, this move = %s, diff = %d\n\n", UCI::move_to_string(lastmove).c_str(), UCI::move_to_string(m).c_str(), -diff);
	      score += (-diff);//(diff < 0 ? -piece_vals[b.piece_on(from)] : piece_vals[b.piece_on(from)]);
	    }
	  // bonus for avoiding the capture from the threat move (from null search)
	  //if (threat != MOVE_NONE && get_to(threat) == get_from(m)) { score += piece_vals[b.piece_on(from)]/2; } //threatgain/2; }
	  
	  // try to boost those quiet checks which are potentially dangerous
	  if (score == (NINF - 1) && b.checks_king(m) && b.is_dangerous(m, p)) score += 1;// piece_vals[b.piece_on(from)];
	  
	  // if the score is still 0, check the piece square tables and score the move based on the to-square-from-sq difference...
	  if (score == (NINF - 1)) 
	    {	      	      
	      score += (square_score(c, p, b.phase(), to) - square_score(c, p, b.phase(), from));
	    }
	  quiets[q_sz++].score = score;
	  //printf("...final score = %d\n",score);
	}
      else if (m == tt_mv && m != MOVE_NONE)
	{
	  use_tt = true;
	  ttmv = tt_mv;
	}
    }
  
  // insertion sort based on score
  if (q_sz > 1) std::sort(quiets, quiets + q_sz, GreaterThan);
  if (c_sz > 1) std::sort(captures, captures + c_sz, GreaterThan);
  
  // NB. really need a better system here...this is terrible, it assumes the user
  // will use nextmove always after "load"...it's very fragile..and, on top of that, the 
  // init routine takes a lot of time to zero the move-list arrays.
  stored_qsz = q_sz;
  stored_csz = c_sz;
  q_sz = 0;
  c_sz = 0;
  /*
  // debug move ordering
  printf("---------------------------------\n");
  b.print();
  print_list();
  printf("---------------------------------\n\n\n");
  */
}

// note : scores are initialized to large negative numbers
// so we want to sort the least negative of the scores to the front of the
// move list..
void MoveSelect::sort(MoveList * ml, int length)
{

	std::sort(ml, ml + length, GreaterThan);

}

bool MoveSelect::nextmove(Node& node, U16& out, bool split)
{
	if (split)
	{
		// call nextmove from the thread data at this node.
		//U16 move;
		//bool do_next = b.get_worker()->currSplitBlock->ms->nextmove(b,out,false);//node->ms->nextmove(b,out,false);
		//node.sb->split_mutex.lock();
		bool do_next = node.sb->ms->nextmove(node, out, false);
		//node.sb->split_mutex.unlock();
		if (out == MOVE_NONE || !do_next) return false;
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
		return do_next;
	}
	if (select_phase >= PHASE_END) return false;
	switch (select_phase)
	{
	case PHASE_TT:
		if (use_tt)
		{
			use_tt = false;
			out = ttmv;
		}
		else out = MOVE_NONE;

		select_phase++;
		break;
	case PHASE_KILLER1:
		out = killers[0];
		select_phase++;
		return true;
	case PHASE_CAPTURE:
		// the "good" captures (scores >= 0)
		//printf("cap phase\n");
		if (captures[c_sz].score >= 0)
		{
			//std::cout << " select:" << SanSquares[get_from(captures[c_sz].m)] + SanSquares[get_to(captures[c_sz].m)] << std::endl;
			out = captures[c_sz].m; c_sz++; return true;
		}
		else if (captures[c_sz].score < 0 && captures[c_sz].score >= NINF && captures[c_sz].m != MOVE_NONE)
		{
			//std::cout << " select:" << SanSquares[get_from(captures[c_sz].m)] + SanSquares[get_to(captures[c_sz].m)] << std::endl;
			out = captures[c_sz].m; c_sz++; return true;
		}

		// if we get here, the previous 2 conditions failed..
		select_phase++;
		out = MOVE_NONE;
		break;

	case PHASE_KILLER2:
		out = killers[1];
		select_phase++;
		return true;
	case PHASE_QUIET:
		//printf("quiet phase : qsz = %d\n",q_sz);
		//std::cout << " select:" << SanSquares[get_from(quiets[q_sz].m)] + SanSquares[get_to(quiets[q_sz].m)] << std::endl;
		out = quiets[q_sz].m; q_sz++;
		if (out == MOVE_NONE)
		{
			select_phase++;
			return false;
		}
		break;
	}
	return true;
}


