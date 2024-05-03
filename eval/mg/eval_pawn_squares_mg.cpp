
#include "../eval.h"
#include "squares_mg.h"

namespace Evaluation {


	int Evaluation::eval_pawn_squares_mg(const Color& c, const position& p) {
		int score = 0;

		Square* sqs = p.squares_of<pawn>(c);
		for (Square s = *sqs; s != no_square; s = *++sqs) {

			score += _pIfo.mg.sq_scaling[pawn] * mg_square_score(pawn, c, Square(s));
			score += _pIfo.mg.material_vals[pawn];
		}

		return score;
	}

}
