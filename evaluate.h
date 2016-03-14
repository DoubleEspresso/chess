#pragma once
#ifndef HEDWIG_EVALUATE_H
#define HEDWIG_EVALUATE_H

#include "board.h"

#include <vector>
#ifdef DEBUG
#include <cassert>
#endif

namespace Eval
{

	extern int evaluate(Board& b);
};


#endif 
