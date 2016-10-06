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
	int eval_draw(Board& b, EvalInfo& ei);
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
template<EndgameType t>
int Endgame::evaluate(Board& b, EvalInfo& ei, int eval)
{
	int eval = 0;
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
	score += eval_passers<WHITE>(b, ei) - eval_passer<BLACK>(b, ei);
	return eval;
}

int Endgame::eval_draw(Board&b, EvalInfo& ei)
{
	int score = 0;
	return score;
}
template<Color c> int Endgame::eval_kk(Board&b, EvalInfo& ei)
{
	int score = 0;
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
