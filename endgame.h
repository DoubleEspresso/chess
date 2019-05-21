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

      if (!(bitboards::squares[occ] & enemies)) {
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
