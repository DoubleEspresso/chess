
#include "../eval.h"

namespace Evaluation {


	int Evaluation::eval_backward_pawn_eg(const Color& c, const position& p) {
		int score = 0;

		// Evaluation points for a backward pawn
		// - secondary weaknesses are much worse with backward pawns
		// - do not allow the square infront of the pawn to be controlled/blocked by the enemy
		// - cannot advance in many scenarios - so mobilization penalties are appropriate
		// - minor allowance if the backward pawn is in the center (but this should be a central eval)

		auto backward = _ifo.backward_pawns[c];
		if (!backward)
			return score;

		auto pawns = p.get_pieces<pawn>(c);
		auto epawns = p.get_pieces<pawn>(Color(c ^ 1));

		while (backward) {

			auto s = bits::pop_lsb(backward);

			// Simple penalty for backward pawns in middle game (they are generally always weak)
			score -= _pIfo.eg.backwardPenalty;

			// Backward pawns with enemy pawns on either neighboring column likely will 
			// not be able to advance easily
			auto neighbors = bitboards::neighbor_cols[s] & epawns;
			if (neighbors)
				score -= _pIfo.eg.backwardNeighborPenalty;

			// Is this pawn blockaded - do we control the 'front sq'
			// Note: we could create a weak squares bb and add this to a weak square eval.
			auto front = (c == white ? s + 8 : s - 8);
			auto eAttks = p.attackers_of2(Square(front), Color(c ^ 1));
			auto fAttks = p.attackers_of2(Square(front), Color(c));
			if (eAttks) {
				auto eCount = bits::count(eAttks);
				auto fCount = bits::count(fAttks);
				if (eCount > fCount)
					score -= _pIfo.eg.backwardControlPenalty;
			}
		}
		return score;
	}

}
