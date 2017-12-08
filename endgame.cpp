#include "endgame.h"
#include "definitions.h"
#include "squares.h"
#include "globals.h"
#include "utils.h"
#include "magic.h"
#include "material.h"
#include "pawns.h"
#include "opts.h"

using namespace Globals;
using namespace Magic;

namespace Endgame
{
  /* main evaluation methods */
  int eval_draw(Board& b, Eval::EvalInfo& ei);
  template<Color c> bool has_opposition(Board& b, int wks, int bks);
  template<Color c> int eval_kk(Board& b, EvalInfo& ei);
  template<Color c> int eval_knnk(Board& b, EvalInfo& ei);
  template<Color c> int eval_kbbk(Board& b, EvalInfo& ei);
  template<Color c> int eval_krrk(Board& b, EvalInfo& ei);
  template<Color c> int eval_kqqk(Board& b, EvalInfo& ei);
  template<Color c> int eval_knbk(Board& b, EvalInfo& ei);
  template<Color c> int eval_knqk(Board& b, EvalInfo& ei);
  template<Color c> int eval_kbrk(Board& b, EvalInfo& ei);
  template<Color c> int eval_kbqk(Board& b, EvalInfo& ei);
  template<Color c> int eval_krnk(Board& b, EvalInfo& ei);
  template<Color c> int eval_krqk(Board& b, EvalInfo& ei);
  template<Color c> int eval_kings(Board& b, EvalInfo& ei);
  template<Color c> int eval_passers(Board& b, EvalInfo& ei);
};

// if we get here, the material count is <= 2 minors + pawns
// which means it is safe (in theory) to call a specialized endgame eval on this position
// the purpose of which is to (mostly) determine of the position is drawn -- ie. rescale material differences closer to 0
// or enhance the score based on promotion/mate considerations specific to the endgame.
int Endgame::evaluate(Board& b, EvalInfo& ei)
{
  int score = 0;
  EndgameType t = ei.me->endgame_type;
  switch (t)
    {
    case KK:				score += (eval_kk<WHITE>(b, ei) - eval_kk<BLACK>(b, ei)); break;
    case KnnK:				score += (eval_knnk<WHITE>(b, ei) - eval_knnk<BLACK>(b, ei)); break;
    case KbbK:				score += (eval_kbbk<WHITE>(b, ei) - eval_kbbk<BLACK>(b, ei)); break;
    case KrrK:				score += (eval_krrk<WHITE>(b, ei) - eval_krrk<BLACK>(b, ei)); break;
    case KqqK:				score += (eval_kqqk<WHITE>(b, ei) - eval_kqqk<BLACK>(b, ei)); break;
    case KnbK: case KbnK:	score += (eval_knbk<WHITE>(b, ei) - eval_knbk<BLACK>(b, ei)); break;
    case KnqK: case KqnK:	score += (eval_knqk<WHITE>(b, ei) - eval_knqk<BLACK>(b, ei)); break;
    case KbrK: case KrbK:	score += (eval_kbrk<WHITE>(b, ei) - eval_kbrk<BLACK>(b, ei)); break;
    case KbqK: case KqbK:	score += (eval_kbqk<WHITE>(b, ei) - eval_kbqk<BLACK>(b, ei)); break;
    case KrnK: case KnrK:	score += (eval_krnk<WHITE>(b, ei) - eval_krnk<BLACK>(b, ei)); break;
    case KrqK: case KqrK:	score += (eval_krqk<WHITE>(b, ei) - eval_krqk<BLACK>(b, ei)); break;
    }
  score += eval_kings<WHITE>(b, ei) - eval_kings<BLACK>(b, ei); 
  score += eval_passers<WHITE>(b, ei) - eval_passers<BLACK>(b, ei);
  return score;
}

int Endgame::eval_draw(Board&b, EvalInfo& ei)
{
  int score = 0;
  return score;
}
template<Color c> bool Endgame::has_opposition(Board& b, int wks, int bks)
{
  int d = (c == b.whos_move() ? 0 : 1);

  if (COL(wks) == COL(bks)) return (col_distance(wks, bks) + d) % 2 == 0;
	
  if (ROW(wks) == ROW(bks)) return (row_distance(wks, bks) + d) % 2 == 0;
	
  if (on_diagonal(wks, bks)) return ((row_distance(wks, bks) + d) % 2 == 0 && (col_distance(wks, bks) + d) % 2 == 0);	
}

template<Color c> int Endgame::eval_kk(Board&b, EvalInfo& ei)
{
  // king and pawn ending
  int score = 0;
  int them = c ^ 1;
  int us = c;
  int from = ei.kingsq[c];

  // assets
  U64 passed_pawns = ei.pe->passedPawns[us];
  //U64 isolated_pawns = ei.pe->isolatedPawns[us];
  U64 backward_pawns = ei.pe->backwardPawns[us];
  U64 doubled_pawns = ei.pe->doubledPawns[us];

  // targets
  U64 chain_bases = ei.pe->chainBase[them];
  U64 undefended_pawns = ei.pe->undefended[them];
  //U64 chain_heads = ei.pe->pawnChainTips[them];

  // do we have opposition
  bool opposition = has_opposition<c>(b, ei.kingsq[WHITE], ei.kingsq[BLACK]) && c == b.whos_move();
  if (opposition)
    {
      //b.print();
      //printf("..%s has opposition\n", b.whos_move() == WHITE ? "w" : "b");
      score += 16;
    }
  else score -= 16;

  // do we have passed pawns
  //if (passed_pawns != 0ULL) score += 2;

  // do we have control of the square in front of the passed pawn
  if (passed_pawns)
    while (passed_pawns)
      {
	int f = pop_lsb(passed_pawns);
	int infrontSq = (c == WHITE ? f + 8 : f - 8);
	U64 attck_front = PseudoAttacksBB(KING, f) & SquareBB[infrontSq];
	if (attck_front != 0ULL)
	  {
	    //if (opposition) score += 16;
	    if (b.whos_move() == c) score += 16;
	  }

	//U64 squares_until_promotion = SpaceInFrontBB[c][f];
	//int sqs_togo = count(squares_until_promotion);
	//if (on_board(infrontSq))
	//{
	//	score += (7 - sqs_togo) * 10;
	//}
      }


  return score;
}
template<Color c> int Endgame::eval_knnk(Board&b, EvalInfo& ei)
{
  int score = 0;
  return score;
}
template<Color c> int Endgame::eval_kbbk(Board&b, EvalInfo& ei)
{
  int score = 0;
  return score;
}
template<Color c> int Endgame::eval_krrk(Board&b, EvalInfo& ei)
{
  int score = 0;
  return score;
}
template<Color c> int Endgame::eval_kqqk(Board&b, EvalInfo& ei)
{
  int score = 0;
  return score;
}
template<Color c> int Endgame::eval_knbk(Board&b, EvalInfo& ei)
{
  int score = 0;
  return score;
}
template<Color c> int Endgame::eval_knqk(Board&b, EvalInfo& ei)
{
  int score = 0;
  return score;
}
template<Color c> int Endgame::eval_kbrk(Board&b, EvalInfo& ei)
{
  int score = 0;
  return score;
}
template<Color c> int Endgame::eval_kbqk(Board&b, EvalInfo& ei)
{
  int score = 0;
  return score;
}
template<Color c> int Endgame::eval_krnk(Board&b, EvalInfo& ei)
{
  int score = 0;
  return score;
}
template<Color c> int Endgame::eval_krqk(Board&b, EvalInfo& ei)
{
  int score = 0;
  return score;
}
template<Color c> int Endgame::eval_kings(Board& b, EvalInfo& ei)
{
  int score = 0;
  return score;
}
template<Color c> int Endgame::eval_passers(Board& b, EvalInfo& ei)
{
  int score = 0;
  return score;
}
