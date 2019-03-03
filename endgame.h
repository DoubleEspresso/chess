#pragma once

#include <vector>
#include <algorithm>

#include "position.h"
#include "evaluate.h"
#include "bits.h"
#include "types.h"

namespace eval {

  // only evaluated from white perspective
  inline bool is_fence(const position& p, einfo& ei) {

    if (ei.pe->semiopen[black] != 0ULL) {
      bits::print(ei.pe->semiopen[black]);
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

      if (!(bitboards::squares[occ] & enemies)) return false;

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
        std::cout << "dbg: breaking @ " << curr << " " << prev << std::endl;
        break;
      }

      side |= util::squares_behind(bitboards::col[util::col(prev)], white, prev);
      side |= util::squares_behind(bitboards::col[util::col(curr)], white, curr);
    }

    bool wk_in = (side & wking);
    bool bk_in = (side & bking);

    if (!(wk_in && !bk_in)) return false;
    

    std::cout << "fence dbg: " << std::endl;
    for (const auto& b : blocked) {
      std::cout << SanSquares[b] << " ";
    }
    std::cout << std::endl;

    bits::print(dbg_bb);
    bits::print(side);
    std::cout << "white king inside: " << wk_in << std::endl;
    std::cout << "black king inside: " << bk_in << std::endl;

    return connected && wk_in && !bk_in;
  }


}
