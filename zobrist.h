/*
-----------------------------------------------------------------------------
This source file is part of the Havoc chess engine
Copyright (c) 2020 Minniesoft
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
#pragma once

#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "types.h"
#include "utils.h"

namespace zobrist {
  bool load();
  U64 piece(const Square& s, const Color& c, const Piece& p);
  U64 castle(const Color& c, const U16& bit);
  U64 ep(const U8& column);
  U64 stm(const Color& c);
  U64 mv50(const U16& count);
  U64 hmvs(const U16& count);
  U64 gen(const unsigned int& bits, util::rand<unsigned int>& r);

  extern U64 piece_rands[Square::squares][2][pieces];
  extern U64 castle_rands[2][16];
  extern U64 ep_rands[8];
  extern U64 stm_rands[2];
  extern U64 move50_rands[512];
  extern U64 hmv_rands[512];
}

#endif
