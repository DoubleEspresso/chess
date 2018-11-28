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

int piece_vals[5] = { PawnValueMG, KnightValueMG, BishopValueMG, RookValueMG, QueenValueMG };

void MoveStats::update(Board& b,
		       U16& m,
		       U16& last,
		       Node* stack,
		       int d,
		       int eval,
		       U16 * quiets) {
  // called when eval >= beta in main search --> last move was a blunder (?)
  if (m == MOVE_NONE) return;
  int f = get_from(m);
  int t = get_to(m);
  int type = int((m & 0xf000) >> 12);
  int c = b.whos_move();
  if (type == QUIET) history[c][f][t] += pow(2, d);
  
  // if we get here, eval >= beta, so last move is most likely a blunder, reduce the history score of it
  if (last != MOVE_NONE) {
    
    int type = int((last & 0xf000) >> 12);
    if (type == QUIET) {
      int f = get_from(last);
      int t = get_to(last);
      history[c = WHITE ? BLACK : WHITE][f][t] -= pow(2, d);
      countermoves[f][t] = m;
    }
  }

  // so-called relative history heuristic as is done in stockfish,
  // we apply it only to confirmed cut nodes .. to "bump" those moves
  // that cause cutoffs but do not appear as frequently in the search.
  if (quiets) {
    for (int j = 0; U16 mv = quiets[j]; ++j) {
      if (m == mv) continue;
      else if (mv == MOVE_NONE) break;
      int f = get_from(mv);
      int t = get_to(mv);
      history[c][f][t] -= pow(2, d);
    }
  }

  // update the stack killers
  if ((type == QUIET) && m != stack->killer[0]) {
    stack->killer[1] = stack->killer[0];
    stack->killer[0] = m;
  }

  // mate killers
  if (eval >= MATE_IN_MAXPLY && m != stack->killer[2]) {
    stack->killer[3] = stack->killer[2];
    stack->killer[2] = m;
  }
}

void MoveSelect::print_list(Board& b) {
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
  printf("     !!DBG moveselect-sorting :: tomove(%s), type(%s), incheck(%s)\n", (b.whos_move() == WHITE ? "white" : "black"), ((int)type == (int)QUIET ? "quiet" : (int)type == (int)CAPTURE ? "capture" : "unknown"), b.in_check() ? "yes" : "no");
  if (captures) {
    printf("...captures...\n");
    MoveList * ml = captures;
    for (; ; ml++) {
      if (ml->m == MOVE_NONE) break;
      std::string s = UCI::move_to_string(ml->m);
      std::cout << "currmove " << s << " " << ml->v << std::endl;
    }
  }
  if (quiets) {
    printf("...quiets...\n");
    MoveList * ml = quiets;
    for (; ; ml++) {
      if (ml->m == MOVE_NONE) break;
      std::string s = UCI::move_to_string(ml->m);
      std::cout << "currmove " << s << " " << ml->v << std::endl;
    }
  }
  printf(".....end....\n\n");

}

void MoveSelect::load_and_sort(MoveGenerator& mvs,
			       Board& b,
			       U16& ttm,
			       Node * stack,
			       MoveType movetype) {
  U16 killer1 = stack->killer[0];
  U16 killer2 = stack->killer[1];
  U16 mate1 = stack->killer[2];
  U16 mate2 = stack->killer[3];
  U16 lastmove = (stack - 1)->currmove;  
  
  for (; !mvs.end(); ++mvs) {
    U16 m = mvs.move();
    
    if (m == ttm) continue;
    if (m == killer1 || m == killer2 || m == mate1 || m == mate2) continue;
    
    int mt = int((m & 0xf000) >> 12);


    // captures
    if (movetype == CAPTURE &&
	(mt == CAPTURE || mt == EP ||
	 (mt <= PROMOTION_CAP && mt > PROMOTION))) {
      captures[c_sz].m = m;
      int score = 0;
      
      if (mt == EP) {
	captures[c_sz++].v = score;
	continue;
      }
      
      captures[c_sz++].v = b.see_sign(m);
    }
    else if ((mt == QUIET || mt == CASTLE_KS || mt == CASTLE_QS || mt <= PROMOTION)) {
      
      quiets[q_sz].m = m;

      // history bonus
      int score = statistics->score(m, b.whos_move());

      // countermove bonus      
      if (lastmove != MOVE_NONE &&
	  m == statistics->countermoves[get_from(lastmove)][get_to(lastmove)]) {
	score += 10;
      }
            
      // square table bonus if score is a minimum
      
      if (score <= NINF) {
	int f = get_from(m);
	int t = get_to(m);
	Piece p = b.piece_on(f);
	int phase = b.phase();	
	int sc = 0;
	if (p != PIECE_NONE) {
	  if (b.whos_move() == WHITE) { sc = square_score<WHITE>(phase, t, p) - square_score<WHITE>(phase, f, p); }
	  else { sc = square_score<BLACK>(phase, t, p) - square_score<BLACK>(phase, f, p); }
	}
	score += sc;
      }
      
      
      quiets[q_sz++].v = score;
    }
  }

  // insertion sort based on score
  stored_qsz = stored_csz = 0;
  if (q_sz > 0 && movetype == QUIET) {
    std::sort(quiets, quiets + q_sz, MLGreater);
    stored_qsz = q_sz;
    q_sz = 0; // reset indices to 0, they index the quiet/capture arrays now     
  }
  else if (c_sz > 0 && movetype == CAPTURE) {
    std::sort(captures, captures + c_sz, MLGreater);
    stored_csz = c_sz;
    c_sz = 0;
  }
  //print_list(b);
}

bool MoveSelect::nextmove(Board &b, Node * stack, U16& ttm, U16& out, bool split)
{
  if (split) {
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
  switch (phase) {
    
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
    if (stored_csz == 0) {
      MoveGenerator mvs;
      if (type == MainSearch) mvs.generate_pseudo_legal(b, CAPTURE);
      else if (type == QsearchCaptures) mvs.generate_qsearch_mvs(b, CAPTURE, genChecks); // only generates checks if givesCheck == true
      load_and_sort(mvs, b, ttm, stack, CAPTURE);
    }
    if (captures[c_sz].v >= 0 &&
	captures[c_sz].m != MOVE_NONE) {
      out = captures[c_sz].m; c_sz++; return true;
    }
    phase++;
    break;
    
  case BadCaptures:
    if (captures[c_sz].v < 0 && captures[c_sz].m != MOVE_NONE) {
      out = captures[c_sz].m; c_sz++; return true;
    }
    phase++;
    break;
    
  case Quiet:
    if (type == QsearchCaptures) {
      if (!b.in_check()) return false;
    }
    
    // if we are in check and have not found an evasion, generate moves even in qsearch
    if (stored_qsz == 0) {
      MoveGenerator mvs;
      if (type == MainSearch) mvs.generate_pseudo_legal(b, QUIET);
      else if (type == QsearchCaptures && b.in_check()) mvs.generate_qsearch_mvs(b, QUIET, genChecks); 
      load_and_sort(mvs, b, ttm, stack, QUIET);
    }
    if (stored_qsz > 0 && quiets[q_sz].m != MOVE_NONE) {
      out = quiets[q_sz].m; q_sz++; return true;
    }
    phase++;
    return false;
  }
  return true;
}


