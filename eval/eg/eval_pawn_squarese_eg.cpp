
#include "../eval.h"
#include "squares_eg.h"

namespace Evaluation {


	int Evaluation::eval_pawn_squares_eg(const Color& c, const position& p) {
		int score = 0;

		Square* sqs = p.squares_of<pawn>(c);
		auto ks = p.king_square(c);
		for (Square s = *sqs; s != no_square; s = *++sqs) {

			// Square score heuristic
			score += _pIfo.eg.sq_scaling[pawn] * eg_square_score(pawn, c, Square(s));

			// Material evaluation
			score += _pIfo.eg.material_vals[pawn];

			// King tropism score (keep king near pawns)
			// based on manhattan distance
			//int dist = std::max(util::row_dist(s, ks), util::col_dist(s, ks));
			//score += _pIfo.eg.pawnKingDist[dist];
		}

		return score;
	}

}
