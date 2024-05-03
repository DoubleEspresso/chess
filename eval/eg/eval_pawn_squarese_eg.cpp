
#include "../eval.h"
#include "squares_eg.h"

namespace Evaluation {


	int Evaluation::eval_pawn_squares_eg(const Color& c, const position& p) {
		int score = 0;

		Square* sqs = p.squares_of<pawn>(c);
		for (Square s = *sqs; s != no_square; s = *++sqs) {

			score += _pIfo.eg.sq_scaling[pawn] * eg_square_score(pawn, c, Square(s));
			score += _pIfo.eg.material_vals[pawn];
		}

		return score;
	}

}
