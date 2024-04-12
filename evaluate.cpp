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
#include <mutex>
#include <thread>

#include "evaluate.h"
#include "squares.h"
#include "magics.h"
#include "endgame.h"
#include "position.h"

namespace eval {
  std::mutex mtx;
  parameters Parameters;
}

namespace {


  float do_eval(const position& p);

  template<Color c> float eval_pawns(const position& p, einfo& ei);
  template<Color c> float eval_knights(const position& p, einfo& ei);
  template<Color c> float eval_bishops(const position& p, einfo& ei);
  template<Color c> float eval_rooks(const position& p, einfo& ei);
  template<Color c> float eval_queens(const position& p, einfo& ei);
  template<Color c> float eval_king(const position& p, einfo& ei);
  template<Color c> float eval_space(const position& p, einfo& ei);
  template<Color c> float eval_center(const position& p, einfo& ei);
  template<Color c> float eval_color(const position& p, einfo& ei);
  template<Color c> float eval_pawn_levers(const position& p, einfo& ei);
  template<Color c> float eval_passed_pawns(const position& p, einfo& ei);
  template<Color c> float eval_flank_attack(const position&p, einfo& ei);
  template<Color c> float eval_kpk(const position& p, einfo& ei);
  template<Color c> float eval_krrk(const position& p, einfo& ei);
  /*
  template<Color c> float eval_knnk(const position& p, info& ei);
  template<Color c> float eval_kbbk(const position& p, info& ei);
  template<Color c> float eval_kqqk(const position& p, info& ei);
  template<Color c> float eval_knbk(const position& p, info& ei);
  template<Color c> float eval_knqk(const position& p, info& ei);
  template<Color c> float eval_kbrk(const position& p, info& ei);
  template<Color c> float eval_kbqk(const position& p, info& ei);
  template<Color c> float eval_krnk(const position& p, info& ei);
  template<Color c> float eval_krqk(const position& p, info& ei);
  template<Color c> float eval_kings(const position& p, info& ei);
  template<Color c> float eval_passers(const position& p, info& ei);
  */

  /*evaluation helpers*/
  template<Color c> bool trapped_rook(const position& p, einfo& ei, const Square& rs, const int& mobility);


  std::vector<float> material_vals{ 100.0, 300.0 , 315.0, 480.0, 910.0 };


  inline float knight_mobility(const unsigned& n) {
    return -50.0f * exp(-float(n)*0.5f) + 20.0f;
    //return -2.0f + 8.33f * log(n + 1); // 7.143f * log(n + 1); // max of ~20
  }

  inline float bishop_mobility(const unsigned& n) {    
    return -50.0f * exp(-float(n)*0.5f) + 20.0f;
    //return -2.0f + 8.33f * log(n + 1); //6.25f * log(n + 1); // max of ~20
  }

  inline float rook_mobility(const unsigned& n) {
    return 0.0f + 1.11f * log(n + 1); // max of ~20
  }

  inline float queen_mobility(const unsigned& n) {
    return 0.0f + 2.22f * log(n + 1); // max of ~20
  }

  float do_eval(const position& p, const float& lazy_margin) {


    float score = 0;
    einfo ei = {};
    memset(&ei, 0, sizeof(einfo));

    {
      // hash table data
      //std::unique_lock<std::mutex> lock(eval::mtx);
      ei.pe = ptable.fetch(p);
      ei.me = mtable.fetch(p);
    }

    ei.all_pieces = p.all_pieces();
    ei.empty = ~p.all_pieces();
    ei.pieces[white] = p.get_pieces<white>();
    ei.pieces[black] = p.get_pieces<black>();
    ei.weak_pawns[white] = ei.pe->doubled[white] | ei.pe->isolated[white] | ei.pe->backward[white];
    ei.weak_pawns[black] = ei.pe->doubled[black] | ei.pe->isolated[black] | ei.pe->backward[black];
    ei.kmask[white] = bitboards::kmask[p.king_square(white)];
    ei.kmask[black] = bitboards::kmask[p.king_square(black)];
    ei.central_pawns[white] = p.get_pieces<white, pawn>() & bitboards::big_center_mask;
    ei.central_pawns[black] = p.get_pieces<black, pawn>() & bitboards::big_center_mask;
    ei.queen_sqs[white] = p.get_pieces<white, queen>();
    ei.queen_sqs[black] = p.get_pieces<black, queen>();


    // closed center
    //U64 all_center_pawns = ei.central_pawns[white] | ei.central_pawns[black];
    //ei.closed_center = bits::count(all_center_pawns) > 4;

    // pawn holes
    ei.pawn_holes[white] = (ei.pe->backward[white] != 0ULL ? ei.pe->backward[white] << 8 : 0ULL);
    ei.pawn_holes[black] = (ei.pe->backward[black] != 0ULL ? ei.pe->backward[black] >> 8 : 0ULL);


    // init score    
    score += ei.pe->score;
    score += ei.me->score;
    score += (p.to_move() == white ? p.params.tempo : -p.params.tempo);
    float pscore = 0;

    // early return on lazy margin (try #1)
    if (lazy_margin > 0 && !ei.me->is_endgame() && abs(score) >= lazy_margin)
    {
      return p.to_move() == white ? score : -score;
    }


    // pure endgame evaluation    
    if (ei.me->is_endgame()) {
      EndgameType t = ei.me->endgame;
      switch (t) {
      case KpK:  score += (eval_kpk<white>(p, ei) - eval_kpk<black>(p, ei)); break;
      case KrrK:  score += (eval_krrk<white>(p, ei) - eval_krrk<black>(p, ei)); break;
        /*
      case KnnK:  score += (eval_knnk<white>(p, ei) - eval_knnk<black>(p, ei)); break;
      case KbbK:  score += (eval_kbbk<white>(p, ei) - eval_kbbk<black>(p, ei)); break;
      case KqqK:  score += (eval_kqqk<white>(p, ei) - eval_kqqk<black>(p, ei)); break;
      case KnbK: case KbnK:	score += (eval_knbk<white>(p, ei) - eval_knbk<black>(p, ei)); break;
      case KnqK: case KqnK:	score += (eval_knqk<white>(p, ei) - eval_knqk<black>(p, ei)); break;
      case KbrK: case KrbK:	score += (eval_kbrk<white>(p, ei) - eval_kbrk<black>(p, ei)); break;
      case KbqK: case KqbK:	score += (eval_kbqk<white>(p, ei) - eval_kbqk<black>(p, ei)); break;
      case KrnK: case KnrK:	score += (eval_krnk<white>(p, ei) - eval_krnk<black>(p, ei)); break;
      case KrqK: case KqrK:	score += (eval_krqk<white>(p, ei) - eval_krqk<black>(p, ei)); break;
      */
      }
    }

    pscore += (eval_color<white>(p, ei) - eval_color<black>(p, ei));
    pscore += (eval_knights<white>(p, ei) - eval_knights<black>(p, ei));
    pscore += (eval_bishops<white>(p, ei) - eval_bishops<black>(p, ei));
    pscore += (eval_rooks<white>(p, ei) - eval_rooks<black>(p, ei));
    pscore += (eval_queens<white>(p, ei) - eval_queens<black>(p, ei));
    pscore += (eval_king<white>(p, ei) - eval_king<black>(p, ei));  

    //pscore += (eval_flank_attack<white>(p, ei) - eval_flank_attack<black>(p, ei));
    pscore += (eval_passed_pawns<white>(p, ei) - eval_passed_pawns<black>(p, ei));

    // early return on lazy margin (try #2)
    //if (lazy_margin > 0 && abs(score) >= lazy_margin)
    //{
    //  return p.to_move() == white ? score : -score;
    //}

    pscore += (eval_space<white>(p, ei) - eval_space<black>(p, ei));
    pscore += (eval_center<white>(p, ei) - eval_center<black>(p, ei));
    pscore += (eval_pawn_levers<white>(p, ei) - eval_pawn_levers<black>(p, ei));
    pscore *= 1.35; // give more weight to positional evaluation
    score += pscore;

    return p.to_move() == white ? score : -score;
  }



  template<Color c> float eval_pawns(const position& p, einfo& ei) {
    float score = 0;
    //U64 pawns = p.get_pieces<c, pawn>();
    //while (pawns) {
    //int f = bits::pop_lsb(pawns);
    //}
    return score;
  }


  template<Color c> float eval_knights(const position& p, einfo& ei) {
    float score = 0;
    Square * knights = p.squares_of<c, knight>();
    Color them = Color(c ^ 1);
    U64 enemies = ei.pieces[them];
    U64 pawn_targets = ei.weak_pawns[them];
    U64 equeen_sq = ei.queen_sqs[them];
    int ks = p.king_square(c);

    for (Square s = *knights; s != no_square; s = *++knights) {
      score += p.params.sq_score_scaling[knight] * square_score<c>(knight, s);

      // mobility
      U64 mvs = bitboards::nmask[s];
      if (!(bitboards::squares[s] & p.pinned<c>())) {
        U64 mobility = (mvs & ei.empty) & (~ei.pe->attacks[them]);
        score += p.params.mobility_scaling[knight] * knight_mobility(bits::count(mobility));
      }

      // outpost (pawn-hole occupation)
      if ((bitboards::squares[s] & ei.pawn_holes[them])) {
        score += p.params.knight_outpost_bonus[util::col(s)];
      }

      // closed center bonus
      if (ei.pe->locked_center || ei.pe->center_pawn_count >= 4)
      {
        score += p.params.bishop_open_center_bonus;
      }

      // bonus for queen attacks
      U64 qattks = mvs & equeen_sq;
      if (qattks) score += p.params.attk_queen_bonus[knight];

      // king distance computation
      int dist = std::max(util::row_dist(s, ks), util::col_dist(s, ks));
      score -= dist;

      // attacks      
      U64 attks = (mvs & enemies) & (~pawn_targets);
      U64 pattks = (mvs & pawn_targets);

      if (attks) {
        while (attks) {
          score += p.params.attack_scaling[knight] *
            p.params.knight_attks[p.piece_on(Square(bits::pop_lsb(attks)))];
        }
      }

      if (pattks) {
        score += p.params.attack_scaling[knight] *
          p.params.knight_attks[pawn] * bits::count(pattks);
      }

      // king harassment
      
      // safe check bonus - can we move from this square and check the enemy king
      U64 scheck_bm = mvs & bitboards::kchecks[p.king_square(them)];
      if (scheck_bm != 0ULL) score += 0.5 * bits::count(scheck_bm);

      U64 kattks = mvs & ei.kmask[them];
      if (kattks) {
        ei.kattackers[c][knight]++; // kattackers of "other" king
        ei.kattk_points[c] |= kattks; // attack points of "other" king
        score += p.params.knight_king[std::min(2, bits::count(kattks))];
      }


      // protected
      U64 support = p.attackers_of2(s, c);
      if (support != 0ULL) {
        score += bits::count(support);
      }
    }
    return score;
  }


  template<Color c> float eval_bishops(const position& p, einfo& ei) {
    float score = 0;
    Square * bishops = p.squares_of<c, bishop>();
    Color them = Color(c ^ 1);
    U64 enemies = ei.pieces[them];
    U64 pawn_targets = ei.weak_pawns[them];
    bool dark_sq = false;
    bool light_sq = false;
    U64 elight_sq_pawns = ei.white_pawns[them];
    U64 flight_sq_pawns = ei.white_pawns[c];
    U64 edark_sq_pawns = ei.black_pawns[them];
    U64 fdark_sq_pawns = ei.black_pawns[c];
    U64 equeen_sq = ei.queen_sqs[them];
    U64 center_pawns = ei.central_pawns[c];
    U64 valuable_enemies = (c == white ?
      p.get_pieces<black, queen>() | p.get_pieces<black, rook>() | p.get_pieces<black, king>() :
      p.get_pieces<white, queen>() | p.get_pieces<white, rook>() | p.get_pieces<white, king>());
    int ks = p.king_square(c);

    for (Square s = *bishops; s != no_square; s = *++bishops) {
      score += p.params.sq_score_scaling[bishop] * square_score<c>(bishop, s);

      if (bitboards::squares[s] & bitboards::colored_sqs[white]) light_sq = true;
      if (bitboards::squares[s] & bitboards::colored_sqs[black]) dark_sq = true;


      // xray bonus
      U64 xray = bitboards::battks[s] & valuable_enemies;
      if (xray) {
        score += bits::count(xray);
      }

      // mobility      
      U64 mvs = magics::attacks<bishop>(ei.all_pieces, s);
      U64 mobility = (mvs & ei.empty) & (~ei.pe->attacks[them]);
      float mscore = p.params.mobility_scaling[bishop] * bishop_mobility(bits::count(mobility));
      if ((bitboards::squares[s] & p.pinned<c>())) mscore /= p.params.pinned_scaling[bishop];

      score += mscore;


      // king distance computation
      int dist = std::max(util::row_dist(s, ks), util::col_dist(s, ks));
      score -= dist;

      // closed center penalty
      if (ei.pe->locked_center || ei.pe->center_pawn_count >= 4)
      {
        score -= p.params.bishop_open_center_bonus;
      }

      // outpost bonus
      if ((bitboards::squares[s] & ei.pawn_holes[them])) {
        score += p.params.bishop_outpost_bonus[util::col(s)];
      }


      // light-square bishop color bonus
      const float same_color_penalty = (ei.me->is_endgame() ? 1.5 : 0.25);
      if (light_sq) {
        // case 1: no opposing bishop to challenge ours + pawn color weaknesses
        //if (elight_sq_pawns == 0ULL || bits::count(elight_sq_pawns) <= 1) {
        //  U64 ew_bishop =
        //    (c == white ? p.get_pieces<black, bishop>() : p.get_pieces<white, bishop>()) &
        //    bitboards::colored_sqs[white];
        //  if (ew_bishop == 0ULL) score += p.params.bishop_color_complex_bonus;
        //}
        
        // case 2: penalty for too many friendly pawns on light-squares
        if (flight_sq_pawns != 0ULL) {
          score -= same_color_penalty * bits::count(flight_sq_pawns);
        }
        if (elight_sq_pawns != 0ULL) {
          score += same_color_penalty * bits::count(elight_sq_pawns);
        }
      }

      //// dark-square bishop color bonus
      if (dark_sq) {
        // case 1: no opposing bishop to challenge ours + pawn color weaknesses
        //if (edark_sq_pawns == 0ULL || bits::count(edark_sq_pawns) <= 1) {
        //  U64 ed_bishop =
        //    (c == white ? p.get_pieces<black, bishop>() : p.get_pieces<white, bishop>()) &
        //    bitboards::colored_sqs[black];
        //  if (ed_bishop == 0ULL) score += p.params.bishop_color_complex_bonus;
        //}

        // case 2: penalty for too many friendly pawns on light-squares
        if (fdark_sq_pawns != 0ULL) {
          score -= same_color_penalty * bits::count(fdark_sq_pawns);
        }
        if (edark_sq_pawns != 0ULL) {
          score += same_color_penalty * bits::count(fdark_sq_pawns);
        }
      }

      // bonus for queen attacks
      U64 qattks = mvs & equeen_sq;
      if (qattks) score += p.params.attk_queen_bonus[bishop];

      // attacks      
      U64 attks = (mvs & enemies) & (~pawn_targets);
      U64 pattks = (mvs & pawn_targets);
      if (attks) {
        while (attks) {
          score += p.params.attack_scaling[bishop] *
            p.params.bishop_attks[p.piece_on(Square(bits::pop_lsb(attks)))];
        }
      }
      if (pattks) {
        score += p.params.attack_scaling[bishop] *
          p.params.bishop_attks[pawn] * bits::count(pattks);
      }


      // king harassment 

      // safe check bonus - can we move from this square and check the enemy king
      U64 scheck_bm = mvs & bitboards::kchecks[p.king_square(them)];
      if (scheck_bm != 0ULL) score += 0.5 * bits::count(scheck_bm);
        
      U64 kattks = mvs & ei.kmask[them];
      if (kattks) {
        ei.kattackers[c][bishop]++;
        ei.kattk_points[c] |= kattks;
        score += p.params.bishop_king[std::min(2, bits::count(kattks))];
      }

      // protected
      U64 support = p.attackers_of2(s, c);
      if (support != 0ULL) {
        score += bits::count(support);
      }

    }
    if (light_sq && dark_sq) score += p.params.doubled_bishop_bonus;

    return score;
  }


  template<Color c> float eval_rooks(const position& p, einfo& ei) {
    float score = 0;
    Square * rooks = p.squares_of<c, rook>();
    Color them = Color(c ^ 1);
    U64 enemies = ei.pieces[them];
    U64 pawn_targets = ei.weak_pawns[them];
    std::vector<Square> Squares;
    U64 equeen_sq = ei.queen_sqs[them];
    U64 valuable_enemies = (c == white ?
      p.get_pieces<black, queen>() | p.get_pieces<black, king>() :
      p.get_pieces<white, queen>() | p.get_pieces<white, king>());

    for (Square s = *rooks; s != no_square; s = *++rooks) {
      score += p.params.sq_score_scaling[rook] * square_score<c>(rook, s);

      Squares.push_back(s);


      // xray bonus
      U64 xray = bitboards::rattks[s] & valuable_enemies;
      if (xray) {
        score += bits::count(xray);
      }

      // mobility      
      U64 mvs = magics::attacks<rook>(ei.all_pieces, s);

      U64 mobility = (mvs & ei.empty) & (~ei.pe->attacks[them]);
      int free_sqs = bits::count(mobility);
      float mscore = p.params.mobility_scaling[rook] * rook_mobility(free_sqs);
      if ((bitboards::squares[s] & p.pinned<c>())) mscore /= p.params.pinned_scaling[rook];
      score += mscore;

      // is our rooked trapped
      if (trapped_rook<c>(p, ei, s, free_sqs)) {

        //std::cout << "!!DEBUG trapped rook on sq: " << SanSquares[s] << std::endl;
        
        score -= p.params.trapped_rook_penalty[ei.me->is_endgame()];

        if (!ei.me->is_endgame() && !p.has_castled<c>()) {
          score -= 2.0f;
        }

      }

      // bonus for queen attacks
      U64 qattks = mvs & equeen_sq;
      if (qattks) score += p.params.attk_queen_bonus[rook];

      // attacks      
      U64 attks = (mvs & enemies) & (~pawn_targets);
      U64 pattks = (mvs & pawn_targets);
      if (attks) {
        while (attks) {
          score += p.params.attack_scaling[rook] *
            p.params.rook_attks[p.piece_on(Square(bits::pop_lsb(attks)))];
        }
      }
      if (pattks) {
        score +=
          p.params.attack_scaling[rook] *
          p.params.rook_attks[pawn] * bits::count(pattks);
      }

      // open file bonus
      U64 column = bitboards::col[util::col(s)] & (p.get_pieces<white, pawn>() | p.get_pieces<black, pawn>());
      if (column == 0ULL) score += p.params.open_file_bonus;

      // 7th rank bonus
      if (bitboards::squares[s] &
        (c == white ? bitboards::row[Row::r7] :
          bitboards::row[Row::r2])) {
        score += p.params.rook_7th_bonus;
      }

      // king harassment
      // safe check bonus - can we move from this square and check the enemy king
      U64 scheck_bm = mvs & bitboards::kchecks[p.king_square(them)];
      if (scheck_bm != 0ULL) score += 0.5 * bits::count(scheck_bm);

      U64 kattks = mvs & ei.kmask[them];
      if (kattks) {
        ei.kattackers[c][rook]++;
        ei.kattk_points[c] |= kattks;
        score += p.params.rook_king[std::min(4, bits::count(kattks))];
      }

      // protected
      U64 support = p.attackers_of2(s, c);
      if (support != 0ULL) {
        score += bits::count(support);
      }

    }

    // connected rooks
    if (Squares.size() >= 2) {
      int row0 = util::row(Squares[0]);
      int row1 = util::row(Squares[1]);
      int col0 = util::col(Squares[0]);
      int col1 = util::col(Squares[1]);

      if ((row0 == row1) || (col0 == col1)) {
        U64 between_bb = bitboards::between[Squares[0]][Squares[1]];
        U64 sq_bb = bitboards::squares[Squares[0]] | bitboards::squares[Squares[1]];
        U64 blockers = (between_bb ^ sq_bb) & ei.all_pieces;

        if (blockers == 0ULL) {
          score += p.params.connected_rook_bonus;
        }
      }
    }

    return score;
  }



  template<Color c> float eval_queens(const position& p, einfo& ei) {
    float score = 0;
    Square * queens = p.squares_of<c, queen>();
    Color them = Color(c ^ 1);
    U64 enemies = ei.pieces[them];
    U64 pawn_targets = ei.weak_pawns[them];

    for (Square s = *queens; s != no_square; s = *++queens) {
      score += p.params.sq_score_scaling[queen] * square_score<c>(queen, s);

      // mobility      
      U64 mvs = (magics::attacks<bishop>(ei.all_pieces, s) |
        magics::attacks<rook>(ei.all_pieces, s));
      U64 mobility = (mvs  & ei.empty) & (~ei.pe->attacks[them]);
      float mscore = p.params.mobility_scaling[queen] * queen_mobility(bits::count(mobility));
      if ((bitboards::squares[s] & p.pinned<c>())) mscore /= p.params.pinned_scaling[queen];
      score += mscore;

      // tempo
      // penalty for being attacked by minor/pawn

      // attacks      
      U64 attks = (mvs & enemies) & (~pawn_targets);
      U64 pattks = (mvs & pawn_targets);

      if (attks) {
        while (attks) {
          score += p.params.attack_scaling[queen] *
            p.params.queen_attks[p.piece_on(Square(bits::pop_lsb(attks)))];
        }
      }
      if (pattks) {
        score += p.params.attack_scaling[queen] *
          p.params.queen_attks[pawn] * bits::count(pattks);
      }

      // king harassment
      // safe check bonus - can we move from this square and check the enemy king
      U64 scheck_bm = mvs & bitboards::kchecks[p.king_square(them)];
      if (scheck_bm != 0ULL) score += 0.5 * bits::count(scheck_bm);

      U64 kattks = mvs & ei.kmask[them];
      if (kattks) {
        ei.kattackers[c][queen]++;
        ei.kattk_points[c] |= kattks;
        score += p.params.queen_king[std::min(6, bits::count(kattks))];
      }
    }

    return score;
  }

	

  template<Color c> float eval_king(const position& p, einfo& ei) {
    float score = 0;
    Square * kings = p.squares_of<c, king>();
    Color them = Color(c ^ 1);
    float attacker_score = 0.0f;

    for (Square s = *kings; s != no_square; s = *++kings) {

      if (!ei.me->is_endgame()) {
        score += p.params.sq_score_scaling[king] * square_score<c>(king, s);
      }

      // mobility      
      U64 mvs = ei.kmask[c] & ei.empty;

      // harassment score
      U64 unsafe_bb = ei.kattk_points[them];  // their attack points to our king

      if (unsafe_bb) {
        mvs &= (~unsafe_bb);
        unsigned num_attackers = 0;

        for (int j = 1; j < 5; ++j) {
          num_attackers += ei.kattackers[them][j];
        }
        attacker_score += 2 * p.params.attacker_weight[std::min((int)num_attackers, 4)];

        score -= attacker_score;

        // number of safe squares
        score += p.params.king_safe_sqs[std::min(7, bits::count(mvs))];
      }

      // pawns around king bonus
      if (!ei.me->is_endgame()) {
        U64 pawn_shelter = ei.pe->king[c] & ei.kmask[c];
        int n = 0;
        if (pawn_shelter) n = std::min(3, bits::count(pawn_shelter));
        score += 0.5 * p.params.king_shelter[n];

        // flat penalty for having pawnless flank in middle game
        U64 kflank = bitboards::kflanks[util::col(s)] & p.get_pieces<c, pawn>();
        if (kflank == 0ULL) score -= 2;
      }

      // malus for king "trapping" rook(s) in corner
      //if (king_traps_rook<c>()) {

      //}

      //if (!p.has_castled<c>()) score -= 20 * p.params.uncastled_penalty;


      // penalty for back rank threats


      // reward having "friends" near the king
      U64 friends = p.get_pieces<c>() & bitboards::kzone[s];
      U64 unfriends = (c == white ? p.get_pieces<black>() & bitboards::kzone[s] :
        p.get_pieces<white>() & bitboards::kzone[s]);

      if (friends != 0ULL) {
        //bits::print(friends);
        score += bits::count(friends);
      }
      if (unfriends != 0ULL) {
        score -= bits::count(unfriends);
      }

    }

    //std::cout << "c = " << c << " score = " << score << std::endl;
    return score;
  }


  template<Color c> float eval_space(const position& p, einfo& ei) {
    float score = 0;

    U64 pawns = p.get_pieces<c, pawn>();
    U64 doubled = ei.pe->doubled[c];
    U64 isolated = ei.pe->isolated[c];

    // remove doubled/isolated pawns
    pawns &= ~(doubled | isolated);

    U64 space = 0ULL;
    while (pawns) {
      int s = bits::pop_lsb(pawns);
      space |= util::squares_behind(bitboards::col[util::col(s)], c, s);
    }

    space &= (bitboards::col[Col::C] | bitboards::col[Col::D] |
      bitboards::col[Col::E] | bitboards::col[Col::F]);


    score += 0.75 * bits::count(space);


    return score;
  }


  template<Color c> float eval_color(const position& p, einfo& ei) {
    float score = 0;
    U64 pawns = p.get_pieces<c, pawn>();

    if (ei.me->is_endgame()) return score;


    ei.white_pawns[c] = pawns & bitboards::colored_sqs[white];
    ei.black_pawns[c] = pawns & bitboards::colored_sqs[black];

    int wpawns_num = 0;
    int bpawns_num = 0;
    
    if (ei.white_pawns[c] != 0ULL) wpawns_num = bits::count(ei.white_pawns[c]);
    if (ei.black_pawns[c] != 0ULL) bpawns_num = bits::count(ei.black_pawns[c]);
    int tot_pawns = wpawns_num + bpawns_num;

    U64 fbishops = p.get_pieces<c, bishop>(); 
    U64 ebishops = (c == white ? p.get_pieces<black, bishop>() : p.get_pieces<white, bishop>());
    bool f_wbishop = (fbishops & bitboards::colored_sqs[white]) != 0ULL;
    bool f_bbishop = (fbishops & bitboards::colored_sqs[black]) != 0ULL;

    bool e_wbishop = (ebishops & bitboards::colored_sqs[white]) != 0ULL;
    bool e_bbishop = (ebishops & bitboards::colored_sqs[black]) != 0ULL;

    // king shelter pawns
    //U64 pawn_shelter = ei.pe->king[c] & ei.kmask[c];

    // penalize if we cannot challenge enemy bishop
    if (wpawns_num <= 2 && tot_pawns > 4 && e_wbishop && !f_wbishop)
    {
      score -= p.params.bishop_color_complex_penalty;
    }

    if (bpawns_num <= 2 && tot_pawns > 4 && e_bbishop && !f_bbishop)
    {
      score -= p.params.bishop_color_complex_penalty;
    }
    

    // penalize if we have too many pawns matching our bishop
    //if (wpawns_num >= 5 && f_wbishop)
    //{
    //  score -= p.params.bishop_penalty_pawns_same_color;
    //}

    //if (bpawns_num >= 5 && f_bbishop)
    //{
    //  score -= p.params.bishop_penalty_pawns_same_color;
    //}


    return score;
  }


  template<Color c> float eval_center(const position& p, einfo& ei) {
    float score = 0;

    U64 center_pawns = ei.central_pawns[c] & bitboards::small_center_mask;
    score += bits::count(center_pawns);


    return score;
  }

  template<Color c> float eval_pawn_levers(const position& p, einfo& ei)
  {
    float score = 0;
    U64 their_pawns = (c == white ? p.get_pieces<black, pawn>() : p.get_pieces<white, pawn>());

    U64 pawn_lever_attacks = ei.pe->attacks[c] & their_pawns;

    while (pawn_lever_attacks) {
      int to = bits::pop_lsb(pawn_lever_attacks);
      if (c == p.to_move()) score += p.params.pawn_lever_score[to];

    }
    return score;
  }


  template<Color c> float eval_flank_attack(const position& p, einfo& ei)
  {
    float score = 0;
    if (!ei.pe->locked_center) return score;

    if (!opposite_side_castling()) return score;

    U64 flank_bb = bitboards::col[A] | bitboards::col[B] | bitboards::col[C] |
      bitboards::col[F] | bitboards::col[G] | bitboards::col[H];
    U64 their_pawns = (c == white ? p.get_pieces<black, pawn>() : p.get_pieces<white, pawn>());

    U64 flank_attacks = ei.pe->attacks[c] & their_pawns & flank_bb;

    // double-counts the pawn lever attack when the center is closed
    while (flank_attacks) {
      int to = bits::pop_lsb(flank_attacks);
      if (c == p.to_move()) score += p.params.pawn_lever_score[to];
    }

    return score;
  }


  template<Color c> float eval_passed_pawns(const position& p, einfo& ei)
  {
    float score = 0;
    U64 passers = ei.pe->passed[c];
    if (passers == 0ULL) {
      return score;
    }

    while (passers)
    {
      Square f = Square(bits::pop_lsb(passers));      
      int row_dist = (c == white ? 8 - util::row(f) : util::row(f));
      
      if (row_dist > 3 || row_dist <= 0) {
        score += p.params.passed_pawn_bonus;
        continue;
      }


      Square front = (c == white ? Square(f + 8) : Square(f - 8));

      // 1. is next square blocked?
      if (p.piece_on(front) == Piece::no_piece) {
        score += 1;
      }

      // 2. is next square attacked?
      U64 our_attackers = 0ULL;
      
      if (util::on_board(front)) our_attackers = p.attackers_of2(front, c);

      if (our_attackers != 0ULL) {
        score += 3 * bits::count(our_attackers);
      }

      // 3. bonus for closer to promotion
      score += (
        row_dist == 3 ? 45 :
        row_dist == 2 ? 90 :
        row_dist == 1 ? 180 : 0);
    }

    return score;

  }

  ////////////////////////////////////////////////////////////////////////////////
  // helper functions for evaluation
  ////////////////////////////////////////////////////////////////////////////////

  template<Color c> bool trapped_rook(const position& p, einfo& ei, const Square& rs, const int& mobility) {

    if (mobility >= 3) return false;

    int ks = p.king_square(c);
    int kcol = util::col(ks);
    int rcol = util::col(rs);

    if ((kcol < Col::E) != (rcol < kcol)) return false;

    return true;
  }

  
  bool opposite_side_castling(const position& p, einfo& ei)
  {
    int kcw = util::col(p.king_square(white));
    int kcb = util::col(p.king_square(black));

    return !ei.me->is_endgame() && 
      p.has_castled<white>() &&
      p.has_castled<black>() &&
      ((kcw < Col::E) != (kcb < Col::E));

  }

  ////////////////////////////////////////////////////////////////////////////////
  // endgame evaluations
  ////////////////////////////////////////////////////////////////////////////////
  template<Color c> float eval_kpk(const position& p, einfo& ei) {
    float score = 0;

    // parameters to move to main parameter tracking thing
    const float pawn_majority_bonus = 16;
    const float opposition_bonus = 4;
    const float advanced_passed_pawn_bonus = 10;
    const float king_proximity_bonus = 2;

    // only evaluate the fence once
    if (!ei.endgame.evaluated_fence) {
      ei.endgame.is_fence = eval::is_fence(p, ei);
      ei.endgame.evaluated_fence = true;

      if (ei.endgame.is_fence) {
        //std::cout << "kpk is a fence position" << std::endl;
        return Score::draw;
      }
      //else std::cout << "kpk is not a fence position" << std::endl;
    }

    // we do not have a fence - evaluate 
    // 1. king opposition and tempo (separate score)
    bool has_opposition = eval::has_opposition<c>(p, ei);


    // 2. pawn majorities 
    //  -- minor bonus for pawn majority
    //  -- if we have a pawn majority, minor bonus for king proximity
    //std::vector<U64> our_pawn_majorities;
    //eval::get_pawn_majorities<c>(p, ei, our_pawn_majorities);
    //for (const auto& m : our_pawn_majorities) {
    //  if (m != 0ULL) {
    //    score += pawn_majority_bonus; // these are winning in many kp endings
    //  }
    //}

    // 3. passed pawns
    //  -- bonus for passed pawns (row dependent)
    //  -- large bonus for king proximity
    U64 passed_pawns = ei.pe->passed[c];
    if (passed_pawns != 0ULL) {
      while (passed_pawns) {

        int f = bits::pop_lsb(passed_pawns);
        score += eval::eval_passed_kpk<c>(p, ei, Square(f), has_opposition);
      }
    }

    // 4. pawn breaks
    // - give a small bonus to our pawn levers
    // - if we have the move
    if (c == p.to_move()) score += eval_pawn_levers<c>(p, ei);

    // 5. opposition (always good)
    if (has_opposition) score += opposition_bonus;

    return score;
  }


  template<Color c> float eval_krrk(const position& p, einfo& ei) {
    float score = 0;

    // parameters to move to main parameter tracking thing
    const float pawn_majority_bonus = 16;
    const float opposition_bonus = 4;
    const float advanced_passed_pawn_bonus = 10;
    const float king_proximity_bonus = 2;


    // we do not have a fence - evaluate 
    // 1. king opposition and tempo (separate score)
    bool has_opposition = eval::has_opposition<c>(p, ei);


    // 2. pawn majorities 
    //  -- minor bonus for pawn majority
    //  -- if we have a pawn majority, minor bonus for king proximity
    //std::vector<U64> our_pawn_majorities;
    //eval::get_pawn_majorities<c>(p, ei, our_pawn_majorities);
    //for (const auto& m : our_pawn_majorities) {
    //  if (m != 0ULL) {
    //    score += pawn_majority_bonus; // these are winning in many kp endings
    //  }
    //}

    // 3. passed pawns
    //  -- bonus for passed pawns (row dependent)
    //  -- large bonus for king proximity
    U64 passed_pawns = ei.pe->passed[c];
    if (passed_pawns != 0ULL) {
      while (passed_pawns) {
        int f = bits::pop_lsb(passed_pawns);
        score += eval::eval_passed_krrk<c>(p, ei, Square(f), has_opposition);
      }
    }

    // 4. open row/col bonus
    // 5.

    return score;
  }

}



namespace eval {
  float evaluate(const position& p, const float& lazy_margin) { return do_eval(p, lazy_margin); }
}
