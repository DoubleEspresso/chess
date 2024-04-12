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

#include <vector>
#include <algorithm>

#include "position.h"
#include "evaluate.h"
#include "bits.h"
#include "types.h"

namespace eval {

  template<Color c>
  inline bool has_opposition(const position& p, einfo& ei) {
    Square wks = p.king_square(white);
    Square bks = p.king_square(black);
    Color tmv = p.to_move();

    int cols = util::col_dist(wks, bks) - 1;
    int rows = util::row_dist(wks, bks) - 1;
    bool odd_rows = ((rows & 1) == 1);
    bool odd_cols = ((cols & 1) == 1);
    
    // distant opposition
    if (cols > 0 && rows > 0)  {      
      return (tmv != c && odd_rows && odd_cols);
    }
    // direct opposition
    else return (tmv != c && (odd_rows || odd_cols));
  }


  template<Color c>
  inline void get_pawn_majorities(const position& p, einfo& ei, std::vector<U64>& majorities) {
    U64 our_pawns = p.get_pieces<c, pawn>();
    U64 their_pawns = (c == white ? p.get_pieces<black, pawn>() :
      p.get_pieces<white, pawn>());

    majorities = { 0ULL, 0ULL, 0ULL, }; 

    // queenside majority
    U64 our_qs = our_pawns & bitboards::pawn_majority_masks[0];
    U64 their_qs = their_pawns & bitboards::pawn_majority_masks[0];
    if (our_qs != 0ULL && their_qs != 0) {
      if (bits::count(our_qs) > bits::count(their_qs)) {
        majorities[0] = our_qs;
      }
    }

    // central majority
    U64 our_c = our_pawns & bitboards::pawn_majority_masks[1];
    U64 their_c = their_pawns & bitboards::pawn_majority_masks[1];
    if (our_c != 0ULL && their_c != 0) {
      if (bits::count(our_c) > bits::count(their_c)) {
        majorities[1] = our_c;
      }
    }

    // kingside majority
    U64 our_ks = our_pawns & bitboards::pawn_majority_masks[2];
    U64 their_ks = their_pawns & bitboards::pawn_majority_masks[2];
    if (our_ks != 0ULL && their_ks != 0) {
      if (bits::count(our_ks) > bits::count(their_ks)) {
        majorities[2] = our_ks;
      }
    }
  }


  template<Color c>
  inline float eval_passed_kpk(const position& p, einfo& ei, const Square& f, const bool& has_opposition) {
    float score = 0;

    const float advanced_passed_pawn_bonus = 15;
    const float good_king_bonus = 5;

    Color them = Color(c ^ 1);

    Square ks = p.king_square(c);
    int row_ks = util::row(ks);
    int col_ks = util::col(ks);

    Square eks = p.king_square(them);
    int row_eks = util::row(eks);
    int col_eks = util::col(eks);

    int row = util::row(f);
    int col = util::col(f);

    Square in_front = Square(f + (c == white ? 8 : -8));

    U64 eks_bb = (bitboards::kmask[f] & bitboards::squares[eks]);
    U64 fks_bb = (bitboards::kmask[f] & bitboards::squares[ks]);

    bool e_control_next = eks_bb != 0ULL;
    bool f_control_next = fks_bb != 0ULL;

    bool f_king_infront = (c == white ? row_ks >= row : row_ks <= row);
    bool e_king_infront = (c == white ? row_eks > row : row_eks < row);

    // edge column draw
    if (col == Col::A || col == Col::H) {
      if (e_control_next) return 0;
      if (col_eks == col && e_king_infront) return 0;
    }

    // case 1. bad king position (enemy king blocking pawn)
    if (e_king_infront && !f_king_infront && e_control_next && !has_opposition) {
      return 0;
    }
    

    // case 2. we control front square and have opposition (winning)
    if (f_control_next && has_opposition) {
      score += good_king_bonus;
    }

    int dist = (c == white ? 8 - row : row) - 1;
    dist = (dist < 0 ? 0 : dist);

    // case 3. we are behind the pawn
    //int f_min_dist = std::min(row_ks, col_ks);
    //int e_min_dist = std::min(row_eks, col_eks);
    bool inside_pawn_box = util::col_dist(eks, f) <= dist;
    
    /*
    {
      // debug info
      std::cout << "color = " << c << std::endl;
      std::cout << "ek col dist = " << util::col_dist(eks, f) << std::endl;
      std::cout << "pp dist = " << dist << std::endl;
      std::cout << "ek inside box = " << inside_pawn_box << std::endl;
      std::cout << "ek in front = " << e_king_infront << std::endl;
      std::cout << "fk in front = " << f_king_infront << std::endl;
    }
    */

    int fk_dist = std::max(util::col_dist(ks, f), util::row_dist(ks, f));
    int ek_dist = std::max(util::col_dist(eks, f), util::row_dist(eks, f));
    bool too_far = fk_dist >= ek_dist;
    if (too_far && !f_king_infront && inside_pawn_box) {
      //std::cout << "...ek catches pawn .. draw here..." << std::endl;
      return 0;
    }

    // bonus for being closer to queening
    switch (dist) {
    case 0: score += 7 * advanced_passed_pawn_bonus;
    case 1: score += 6 * advanced_passed_pawn_bonus;
    case 2: score += 5 * advanced_passed_pawn_bonus;
    case 3: score += 4 * advanced_passed_pawn_bonus;
    case 4: score += 3 * advanced_passed_pawn_bonus;
    case 5: score += 2 * advanced_passed_pawn_bonus;
    case 6: score += advanced_passed_pawn_bonus;
    }

    return score;
  }

  template<Color c>
  inline float eval_passed_krrk(const position& p, einfo& ei, const Square& f, const bool& has_opposition) {
    float score = 0;

    const float advanced_passed_pawn_bonus = 15;
    const float rook_behind_pawn_bonus = 10;
    const float free_king_row_bonus = 4;
    const float free_king_col_bonus = 4;
    const float good_king_bonus = 5;

    Color them = Color(c ^ 1);

    Square ks = p.king_square(c);
    int row_ks = util::row(ks);
    int col_ks = util::col(ks);

    Square eks = p.king_square(them);
    int row_eks = util::row(eks);
    int col_eks = util::col(eks);

    int row = util::row(f);
    int col = util::col(f);

    Square frs = p.squares_of<c, rook>()[0];
    int col_fr = util::col(frs);
    int row_fr = util::row(frs);

    Square ers = (c == white ? p.squares_of<black, rook>()[0] :
      p.squares_of<white, rook>()[0]);
    int col_er = util::col(ers);
    int row_er = util::row(ers);


    Square in_front = Square(f + (c == white ? 8 : -8));

    U64 eks_bb = (bitboards::kmask[f] & bitboards::squares[eks]);
    U64 fks_bb = (bitboards::kmask[f] & bitboards::squares[ks]);

    bool e_control_next = eks_bb != 0ULL;
    bool f_control_next = fks_bb != 0ULL;

    bool f_king_infront = (c == white ? row_ks >= row : row_ks <= row);
    bool e_king_infront = (c == white ? row_eks > row : row_eks < row);

    // 1. does our king control the next square
    if (f_control_next && has_opposition) {
      score += good_king_bonus;
    }

    // 2. is our rook behind the passed pawn
    bool fr_behind = (c == white ? row_fr < row : row_fr > row);
    bool same_column = col_fr == col;
    if (fr_behind && same_column) {
      score += rook_behind_pawn_bonus;
    }

    // 3. can our king walk forward (rook is not blocking)
    bool er_row_block = util::row_dist(ers, ks) <= 1;
    if (!er_row_block) {
      score += free_king_row_bonus;
    }

    // 4. can our king walk to our pawn (accross cols)
    // without being blocked by the enemy rook?
    bool bad_order_1 = col_ks < col_er && col_ks < col;
    bool bad_order_2 = col < col_er && col < col_ks;
    if (!bad_order_1 && !bad_order_2) {
      score += free_king_col_bonus;
    }

    // bonus for being closer to queening
    int dist = (c == white ? 8 - row : row) - 1;
    dist = (dist < 0 ? 0 : dist);
    switch (dist) {
    case 1: score += 6 * advanced_passed_pawn_bonus;
    case 2: score += 5 * advanced_passed_pawn_bonus;
    case 3: score += 4 * advanced_passed_pawn_bonus;
    case 4: score += 3 * advanced_passed_pawn_bonus;
    case 5: score += 2 * advanced_passed_pawn_bonus;
    case 6: score += advanced_passed_pawn_bonus;
    }

    return score;
  }


  // only evaluated from white perspective
  inline bool is_fence(const position& p, einfo& ei) {

    if (ei.pe->semiopen[black] != 0ULL) {
      //bits::print(ei.pe->semiopen[black]);
      //std::cout << "..dbg semi-open file in pawn-entry for black, not a fence pos" << std::endl;
      return false;
    }

    U64 enemies = p.get_pieces<black, pawn>();
    if (enemies == 0ULL) return false;

    U64 attacks = ei.pe->attacks[black];
    U64 friends = p.get_pieces<white, pawn>();
    U64 wking = p.get_pieces<white, king>();
    U64 bking = p.get_pieces<black, king>();

    std::vector<int> delta{ -1, 1 };
    std::vector<Square> blocked;

    U64 dbg_bb = 0ULL;

    while (friends) {
      Square start = Square(bits::pop_lsb(friends));

      Square occ = Square(start + 8);

      if (!(bitboards::squares[occ] & enemies) && 
          !(bitboards::squares[occ] & bking)) {
        //std::cout << "..dbg detected pawn-chain is not \"locked\", not a fence position" << std::endl;
        return false;
      }

      blocked.push_back(start);
      dbg_bb |= bitboards::squares[start];

      for (const auto& d : delta) {
        Square n = Square(start + d);

        if (!util::on_board(n)) continue;

        if (bitboards::squares[n] & attacks) {
          blocked.push_back(n);
          dbg_bb |= bitboards::squares[n];
        }       
      }
    }

    if (blocked.size() <= 0) return false;

    Col start_col = Col(util::col(blocked[0]));
    bool connected = (start_col == Col::A) || (start_col == Col::B);

    if (!connected) return false;

    U64 side = 0ULL;

    std::sort(std::begin(blocked), std::end(blocked),
      [](const Square& s1, const Square& s2) -> bool { return util::col(s1) < util::col(s2);  }
    );

    for (int i = 1; i < blocked.size(); ++i) {
      Square curr = blocked[i];
      Square prev = blocked[i - 1];

      connected = (
        (abs(curr - prev) == 1) ||
        (abs(curr - prev) == 8) ||
        (abs(curr - prev) == 7) ||
        (abs(curr - prev) == 9));


      if (!connected) {
        //std::cout << "dbg: breaking @ " << curr << " " << prev << std::endl;
        break;
      }

      side |= util::squares_behind(bitboards::col[util::col(prev)], white, prev);
      side |= util::squares_behind(bitboards::col[util::col(curr)], white, curr);
    }

    bool wk_in = (side & wking);
    bool bk_in = (side & bking);

    if (!(wk_in && !bk_in)) return false;
    
    /*
    std::cout << "fence dbg: " << std::endl;
    for (const auto& b : blocked) {
      std::cout << SanSquares[b] << " ";
    }
    std::cout << std::endl;

    bits::print(dbg_bb);
    bits::print(side);
    std::cout << "white king inside: " << wk_in << std::endl;
    std::cout << "black king inside: " << bk_in << std::endl;
    */

    return connected && wk_in && !bk_in;
  }


}
