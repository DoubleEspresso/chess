
#include "evaluate.h"
#include "squares.h"
#include "magics.h"



namespace {
  
  float do_eval(const position& p);

  template<Color c> float eval_pawns(const position& p, info& ei);
  template<Color c> float eval_knights(const position& p, info& ei);
  template<Color c> float eval_bishops(const position& p, info& ei);
  template<Color c> float eval_rooks(const position& p, info& ei);
  template<Color c> float eval_queens(const position& p, info& ei);
  template<Color c> float eval_king(const position& p, info& ei);

  
  std::vector<float> sq_score_scaling { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };

  std::vector<float> pawn_scaling { 0.86, 0.90, 0.95, 1.00, 1.00, 0.95, 0.90, 0.86 };

  std::vector<float> material_vals { 100.0, 315.0 , 345.0, 530.0, 1020.0 };



  float do_eval(const position& p) {
    
    float score = 0;
    info ei = {};

    score += (eval_pawns<white>(p, ei) - eval_pawns<black>(p, ei));
    score += (eval_knights<white>(p, ei) - eval_knights<black>(p, ei));
    score += (eval_bishops<white>(p, ei) - eval_bishops<black>(p, ei));
    score += (eval_rooks<white>(p, ei) - eval_rooks<black>(p, ei));
    score += (eval_queens<white>(p, ei) - eval_queens<black>(p, ei));
    score += (eval_king<white>(p, ei) - eval_king<black>(p, ei));

    return p.to_move() == white ? score : -score;
  }


  
  template<Color c> float eval_pawns(const position& p, info& ei) {
    float score = 0;
    U64 pawns = p.get_pieces<c, pawn>();
    while (pawns) {
      int f = bits::pop_lsb(pawns);
      score += sq_score_scaling[pawn] * square_score<c>(pawn, Square(f));
      score += pawn_scaling[util::col(f)] * material_vals[pawn];
    }
    return score;
  }

  
  template<Color c> float eval_knights(const position& p, info& ei) {
    float score = 0;    
    Square * knights = p.squares_of<c, knight>();
    
    for (Square s = *knights; s != no_square; s = *++knights) {
      score += material_vals[knight];
      score += sq_score_scaling[knight] * square_score<c>(knight, s);      
    }
    
    return score;
  }

  
  template<Color c> float eval_bishops(const position& p, info& ei) {
    float score = 0;    
    Square * bishops = p.squares_of<c, bishop>();
    
    for (Square s = *bishops; s != no_square; s = *++bishops) {
      score += material_vals[bishop];
      score += sq_score_scaling[bishop] * square_score<c>(bishop, s);      
    }
    
    return score;
  }
  
  
  template<Color c> float eval_rooks(const position& p, info& ei) {
    float score = 0;    
    Square * rooks = p.squares_of<c, rook>();
    
    for (Square s = *rooks; s != no_square; s = *++rooks) {
      score += material_vals[rook];
      score += sq_score_scaling[rook] * square_score<c>(rook, s);      
    }
    
    return score;
  }


  
  template<Color c> float eval_queens(const position& p, info& ei) {
    float score = 0;    
    Square * queens = p.squares_of<c, queen>();
    
    for (Square s = *queens; s != no_square; s = *++queens) {
      score += material_vals[queen];
      score += sq_score_scaling[queen] * square_score<c>(queen, s);      
    }
    
    return score;
  }



  template<Color c> float eval_king(const position& p, info& ei) {
    float score = 0;    
    Square * kings = p.squares_of<c, king>();
    
    for (Square s = *kings; s != no_square; s = *++kings) {      
      score += sq_score_scaling[king] * square_score<c>(king, s);      
    }
    
    return score;
  }

  
};




namespace eval {
  float evaluate(const position& p) { return do_eval(p); }
};
