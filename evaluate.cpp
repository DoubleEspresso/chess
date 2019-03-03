
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

  float do_eval(const position& p) {
    
    float score = 0;
    einfo ei = {};


    // hash table data
    ei.pe = ptable.fetch(p);
    ei.me = mtable.fetch(p);
    score += ei.pe->score;
    score += ei.me->score;    

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
    
    for (Square s = *knights; s != no_square; s = *++knights) {
      score += sq_score_scaling[knight] * square_score<c>(knight, s);      
    }
    
    return score;
  }

  
  template<Color c> float eval_bishops(const position& p, einfo& ei) {
    float score = 0;    
    Square * bishops = p.squares_of<c, bishop>();
    
    for (Square s = *bishops; s != no_square; s = *++bishops) {
      score += sq_score_scaling[bishop] * square_score<c>(bishop, s);      
    }
    
    return score;
  }
  
  
  template<Color c> float eval_rooks(const position& p, einfo& ei) {
    float score = 0;    
    Square * rooks = p.squares_of<c, rook>();
    
    for (Square s = *rooks; s != no_square; s = *++rooks) {
      score += sq_score_scaling[rook] * square_score<c>(rook, s);      
    }
    
    return score;
  }


  
  template<Color c> float eval_queens(const position& p, einfo& ei) {
    float score = 0;    
    Square * queens = p.squares_of<c, queen>();
    
    for (Square s = *queens; s != no_square; s = *++queens) {
      score += sq_score_scaling[queen] * square_score<c>(queen, s);      
    }
    
    return score;
  }



  template<Color c> float eval_king(const position& p, einfo& ei) {
    float score = 0;    
    Square * kings = p.squares_of<c, king>();
    
    for (Square s = *kings; s != no_square; s = *++kings) {      
      score += sq_score_scaling[king] * square_score<c>(king, s);      
    }
    
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
