
#include "../eval.h"
#include "../eg/squares_eg.h"
#include "../mg/squares_mg.h"

namespace Evaluation {

	namespace {
		inline bool has_opposition(const Color& c, const position& p) {
			Square wks = p.king_square(white);
			Square bks = p.king_square(black);
			Color tmv = p.to_move();

			int cols = util::col_dist(wks, bks) - 1;
			int rows = util::row_dist(wks, bks) - 1;
			bool odd_rows = ((rows & 1) == 1);
			bool odd_cols = ((cols & 1) == 1);

			// distant opposition
			if (cols > 0 && rows > 0) {
				return (tmv != c && odd_rows && odd_cols);
			}
			// direct opposition
			else return (tmv != c && (odd_rows || odd_cols));
		}
	}

	// This evaluation represents a generic tally of king features suitable
	// to endgame positions - care is taken to account for danger if queens are on the board
	// More specialized king evaluations are done in pawn and specialized material endgame evals.
	int Evaluation::eval_king_eg(const Color& c, const position& p) {

		// If enemy queens on the board during egame (could restrict to queens + open files):
		//  - use mg piece squares
		//  - weight safety "higher"
		//  - include pawn shelter/pawn affinity

		// - better positioned king (more advanced, centralized)
		// - king pawn tropism (weighted average based on manhattan distance)

		// Note: for certain endgames (kpk) much of the safety pieces of this eval can be dropped.


		int score = 0;
		auto them = Color(c ^ 1);
		auto eQueens = p.get_pieces<queen>(Color(c ^ 1));
		auto enemyPawns = p.get_pieces<pawn>(them);

		Square* kings = p.squares_of<king>(c);
		for (Square s = *kings; s != no_square; s = *++kings) {

			//score += _pIfo.eg.sq_scaling[king] * (eQueens != 0ULL ? mg_square_score(king, c, s) : eg_square_score(king, c, s));

			// Mobility
			// TODO: trim this by piece attacks/safe squares once those are computed incrementally
			//U64 mvs = bitboards::kmask[s] & (~p.all_pieces());
			//score += _pIfo.eg.kingMobility[bits::count(mvs)];

			// Attacks to king zone
			// Pawn shelter

			if (!eQueens) {
				// Bonus for centralized king
				if (bitboards::squares[s] & bitboards::big_center_mask)
					score += _pIfo.eg.centralizedKingBonus;
			}

			// Pawn affinity bonus
		}

		// Opposition bonus
		//if (has_opposition(c, p))
		//	score += _pIfo.eg.kingOppositionBonus;

		return score;
	}

}
