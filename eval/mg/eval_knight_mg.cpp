
#include "../eval.h"
#include "squares_mg.h"

namespace Evaluation {

	namespace {
		/*Specialized debug methods*/
		/*Specialized square scores*/
		/*Specialized utility methods*/
	}

	int Evaluation::eval_knight_mg(const Color& c, const position& p) {
		int score = 0;
		Square* knights = p.squares_of<knight>(c);
		//Color them = Color(c ^ 1);
		//U64 enemies = _ifo.pieces[them];
		//U64 pawn_targets = (c == white ? p.get_pieces<black, pawn>() : p.get_pieces<white, pawn>());
		//U64 equeen_sq = ei.queen_sqs[them];
		//int ks = p.king_square(c);

		for (Square s = *knights; s != no_square; s = *++knights)
			score += _pIfo.mg.knightSqScale * (c == white ? 
				mg_square_score<white>(knight, s) :
				mg_square_score<black>(knight, s));
				

		return score;
	}
}
