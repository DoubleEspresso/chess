#pragma once
#ifndef HEDWIG_ENDGAME_H
#define HEDWIG_ENDGAME_H

#include "evaluate.h"

#include <vector>

using namespace Eval;

namespace Endgame
{
	template<EndgameType t> int evaluate(Board& b, EvalInfo& ei, int eval);
};

#endif