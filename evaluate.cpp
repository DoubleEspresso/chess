
#include "evaluate.h"
#include "squares.h"
#include "magics.h"
#include "endgame.h"

namespace {
  
    
  float do_eval(const position& p);

  template<Color c> float eval_pawns(const position& p, einfo& ei);
  template<Color c> float eval_knights(const position& p, einfo& ei);
  template<Color c> float eval_bishops(const position& p, einfo& ei);
  template<Color c> float eval_rooks(const position& p, einfo& ei);
  template<Color c> float eval_queens(const position& p, einfo& ei);
  template<Color c> float eval_king(const position& p, einfo& ei);
  //template<Color c> float eval_material(const position&p, info& ei);

  template<Color c> float eval_kpk(const position& p, einfo& ei);
  /*
  template<Color c> float eval_kxk(const position&p, info& e); // check for draws here
  template<Color c> float eval_knnk(const position& p, info& ei);
  template<Color c> float eval_kbbk(const position& p, info& ei);
  template<Color c> float eval_krrk(const position& p, info& ei);
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
  std::vector<float> sq_score_scaling { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };

  std::vector<float> material_vals { 100.0, 300.0 , 315.0, 480.0, 910.0 };

  eval::parameters params;

  inline float knight_mobility(const unsigned& n) {
    return -2.0f + 8.33f * log(n + 1); // 7.143f * log(n + 1); // max of ~20
  }

  inline float bishop_mobility(const unsigned& n) {
    return -2.0f + 8.33f * log(n + 1); //6.25f * log(n + 1); // max of ~20
  }

  inline float rook_mobility(const unsigned& n) {
    return 0.0f + 1.11f * log(n + 1); // max of ~20
  }

  inline float queen_mobility(const unsigned& n) {
    return 0.0f + 2.22f * log(n + 1); // max of ~20
  }

  float do_eval(const position& p) {

    float score = 0;
    einfo ei = {};
    memset(&ei, 0, sizeof(einfo));

    // hash table data
    ei.pe = ptable.fetch(p);
    ei.me = mtable.fetch(p);

    ei.all_pieces = p.all_pieces();
    ei.empty = ~p.all_pieces();
    ei.pieces[white] = p.get_pieces<white>();
    ei.pieces[black] = p.get_pieces<black>();
    ei.weak_pawns[white] = ei.pe->doubled[white] | ei.pe->isolated[white];
    ei.weak_pawns[black] = ei.pe->doubled[black] | ei.pe->isolated[black];
    ei.kmask[white] = bitboards::kmask[p.king_square(white)];
    ei.kmask[black] = bitboards::kmask[p.king_square(black)];
    
    // init score
    score += ei.pe->score;
    score += ei.me->score;    
    score += (p.to_move() == white ? params.tempo : -params.tempo);

    // pure endgame evaluation    
    if (ei.me->is_endgame()) {
      EndgameType t = ei.me->endgame;
      switch (t) {
      //case KpK:  score += (eval_kpk<white>(p, ei) - eval_kpk<black>(p, ei)); break;
        /*
      case KnnK:  score += (eval_knnk<white>(p, ei) - eval_knnk<black>(p, ei)); break;
      case KbbK:  score += (eval_kbbk<white>(p, ei) - eval_kbbk<black>(p, ei)); break;
      case KrrK:  score += (eval_krrk<white>(p, ei) - eval_krrk<black>(p, ei)); break;
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

    score += (eval_knights<white>(p, ei) - eval_knights<black>(p, ei));
    score += (eval_bishops<white>(p, ei) - eval_bishops<black>(p, ei));
    score += (eval_rooks<white>(p, ei) - eval_rooks<black>(p, ei));
    score += (eval_queens<white>(p, ei) - eval_queens<black>(p, ei));
    score += (eval_king<white>(p, ei) - eval_king<black>(p, ei));
    
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

    for (Square s = *knights; s != no_square; s = *++knights) {
      score += sq_score_scaling[knight] * square_score<c>(knight, s);
      
      // mobility
      U64 mvs = bitboards::nmask[s];
      U64 mobility = (mvs & ei.empty) ^ ei.pe->attacks[them];
      score += params.mobility_scaling[knight] * knight_mobility(bits::count(mobility));

      // attacks
      U64 attks = (mvs & enemies) & (~pawn_targets);
      U64 pattks = (mvs & pawn_targets);
      if (attks) {
        while (attks) {
          score += params.knight_attks[p.piece_on(Square(bits::pop_lsb(attks)))];
        }
      }
      if (pattks) {
        score += params.knight_attks[pawn] * bits::count(pattks);
      }

      // king harassment
     // std::cout << c << " knight" << std::endl;
     // bits::print(ei.kmask[them]);

      //U64 kattks = mvs & ei.kmask[them];
      //if (kattks) {

      //  //bits::print(kattks);
      //  ei.kattackers[c][knight]++; // kattackers of "other" king
      //  ei.kattk_points[c] |= kattks; // attack points of "other" king
      //  score += params.knight_king[bits::count(kattks)];
      //}


    }
    
    return score;
  }

  
  template<Color c> float eval_bishops(const position& p, einfo& ei) {
    float score = 0;    
    Square * bishops = p.squares_of<c, bishop>();
    Color them = Color(c ^ 1);
    U64 enemies = ei.pieces[them];
    U64 pawn_targets = ei.weak_pawns[them];

    for (Square s = *bishops; s != no_square; s = *++bishops) {
      score += sq_score_scaling[bishop] * square_score<c>(bishop, s);

      // mobility
      U64 mvs = magics::attacks<bishop>(ei.all_pieces, s);
      U64 mobility = (mvs & ei.empty) & (~ei.pe->attacks[them]);
      score += params.mobility_scaling[bishop] * bishop_mobility(bits::count(mobility));

      // attacks
      U64 attks = (mvs & enemies) & (~pawn_targets);
      U64 pattks = (mvs & pawn_targets);
      if (attks) {
        while (attks) {
          score += params.bishop_attks[p.piece_on(Square(bits::pop_lsb(attks)))];
        }
      }
      if (pattks) {
        score += params.bishop_attks[pawn] * bits::count(pattks);
      }

      // king harassment
      //U64 kattks = mvs & ei.kmask[them];
      //if (kattks) {
      //  //std::cout << "bishop" << std::endl;
      //  //bits::print(kattks);
      //  ei.kattackers[c][bishop]++;
      //  ei.kattk_points[c] |= kattks;
      //  score += params.bishop_king[bits::count(kattks)];
      //}

    }
    
    return score;
  }
  
  
  template<Color c> float eval_rooks(const position& p, einfo& ei) {
    float score = 0;    
    Square * rooks = p.squares_of<c, rook>();
    Color them = Color(c ^ 1);
    U64 enemies = ei.pieces[them];
    U64 pawn_targets = ei.weak_pawns[them];

    for (Square s = *rooks; s != no_square; s = *++rooks) {
      score += sq_score_scaling[rook] * square_score<c>(rook, s);      

      // mobility
      U64 mvs = magics::attacks<rook>(ei.all_pieces, s);
      U64 mobility = (mvs & ei.empty) & (~ei.pe->attacks[them]);
      score += params.mobility_scaling[rook] * rook_mobility(bits::count(mobility));

      // attacks
      U64 attks = (mvs & enemies) & (~pawn_targets);
      U64 pattks = (mvs & pawn_targets);
      if (attks) {
        while (attks) {
          score += params.rook_attks[p.piece_on(Square(bits::pop_lsb(attks)))];
        }
      }
      if (pattks) {
        score += params.rook_attks[pawn] * bits::count(pattks);
      }
      
      // king harassment
      //U64 kattks = mvs & ei.kmask[them];
      //if (kattks) {
      //  //std::cout << "rook" << std::endl;
      //  //bits::print(kattks);
      //  ei.kattackers[c][rook]++;
      //  ei.kattk_points[c] |= kattks;
      //  score += params.rook_king[bits::count(kattks)];
      //}

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
      score += sq_score_scaling[queen] * square_score<c>(queen, s);

      // mobility
      U64 mvs = (magics::attacks<bishop>(ei.all_pieces, s) |
        magics::attacks<rook>(ei.all_pieces, s));
      U64 mobility = (mvs  & ei.empty) & (~ei.pe->attacks[them]);
      score += params.mobility_scaling[queen] * queen_mobility(bits::count(mobility));

      // attacks
      U64 attks = (mvs & enemies) & (~pawn_targets);
      U64 pattks = (mvs & pawn_targets);

      if (attks) {
        while (attks) {
          score += params.queen_attks[p.piece_on(Square(bits::pop_lsb(attks)))];
        }
      }
      if (pattks) {
        score += params.queen_attks[pawn] * bits::count(pattks);
      }

      // king harassment
      //U64 kattks = mvs & ei.kmask[them];
      //if (kattks) {
      //  //std::cout << "queen" << std::endl;
      //  //bits::print(kattks);
      //  ei.kattackers[c][queen]++;
      //  ei.kattk_points[c] |= kattks;
      //  score += params.queen_king[bits::count(kattks)];
      //}

      
    }
    
    return score;
  }



  template<Color c> float eval_king(const position& p, einfo& ei) {
    float score = 0;    
    Square * kings = p.squares_of<c, king>();
    Color them = Color(c ^ 1);
    float attacker_score = 0.0f;

    for (Square s = *kings; s != no_square; s = *++kings) {      
      score += sq_score_scaling[king] * square_score<c>(king, s);

      // tempo adjustments
      if (bitboards::squares[s] & ei.pe->attacks[them]) score -= params.tempo;

      // mobility
      //U64 mvs = ei.kmask[c] & ei.empty;

      //// harassment score
      //U64 unsafe_bb = ei.kattk_points[them];  // their attack points to our king
      //if (unsafe_bb) {
      //  mvs &= (~unsafe_bb);
      //  unsigned num_attackers = 0;
      //  for (int j = 1; j < 5; ++j) {
      //    num_attackers += ei.kattackers[them][j];
      //  }
      //  attacker_score += params.attacker_weight[std::min((int)num_attackers, 4)];

      //  //ei.kattackers[them][j] * params.attacker_weight[j];
      //  /*
      //  if (c == black) {
      //    std::cout << "king" << std::endl;
      //    bits::print(unsafe_bb);
      //    bits::print(mvs);
      //    std::cout << "num_attackers = " << num_attackers << " attacker score = " << attacker_score << std::endl;
      //    std::cout << "num_safe = " << (int)(bits::count(mvs)) << std::endl;
      //  }
      //  */
      //  score -= attacker_score;

      //  int num_safe = bits::count(mvs);
      //  if (num_safe <= 0) score -= attacker_score;// num_attackers;
      //}

      //// reward having "good" pawns around the king
      //U64 pawn_shelter = ei.pe->king[c] & ei.kmask[c];
      //if (pawn_shelter) {
      //  int n = std::min(3, bits::count(pawn_shelter)) - 1;
      //  //if (c == black) std::cout << "n = " << n << std::endl;
      //  score += params.king_shelter[n];
      //}

      // reward having "friends" near the king
      
    }
    
    //std::cout << "c = " << c << " score = " << score << std::endl;
    return score;
  }

  


  template<Color c> float eval_kpk(const position& p, einfo& ei) {
    float score = 0;
    p.print();
    std::cout << "evaluating endgame kpk" << std::endl;
    // only evaluate the fence once
    if (!ei.endgame.evaluated_fence) {
      ei.endgame.is_fence = eval::is_fence(p, ei);
      ei.endgame.evaluated_fence = true;

      if (ei.endgame.is_fence) {
        std::cout << "kpk is a fence position" << std::endl;
        return Score::draw;
      }
      else std::cout << "kpk is not a fence position" << std::endl;
    }

    // we do not have a fence - evaluate 
    // 1. king opposition and tempo (separate score)
    // 2. pawn majorities 
    // 3. passed pawns
    // 4. pawn breaks
    // 5. others?
   
    return score;
  }
}




namespace eval {
  float evaluate(const position& p) { return do_eval(p); }
}
