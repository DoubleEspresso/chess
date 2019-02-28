#pragma once

#ifndef EVALUATE_H
#define EVALUATE_H

#include "types.h"
#include "utils.h"
#include "position.h"
#include "pawns.h"

namespace eval {

  float evaluate(const position& p);
}

#endif
