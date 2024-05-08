
#include "../eval.h"

namespace Evaluation {


	namespace {
		const int pawn_lever_score_eg[64] =
		{
			1, 2, 3, 4, 4, 3, 2, 1,
			1, 2, 3, 4, 4, 3, 2, 1,
			1, 2, 3, 4, 4, 3, 2, 1,
			1, 2, 3, 4, 4, 3, 2, 1,
			1, 2, 3, 4, 4, 3, 2, 1,
			1, 2, 3, 4, 4, 3, 2, 1,
			1, 2, 3, 4, 4, 3, 2, 1,
			1, 2, 3, 4, 4, 3, 2, 1
		};
	}

	int Evaluation::eval_undermine_eg(const Color& c, const position& p) {
		float score = 0;
		U64 their_pawns = p.get_pieces<pawn>(Color(c ^ 1));

		// The enemy pawns about to be captured by our pawns
		U64 pawn_lever_attacks = _ifo.pawn_attacks[c] & their_pawns;

		while (pawn_lever_attacks) {
			int to = bits::pop_lsb(pawn_lever_attacks);
			if (c == p.to_move())
				score += pawn_lever_score_eg[to];

			// If these are base chain pawns add a bonus
			if (_ifo.pawnInfo.chainBases[c ^ 1] & bitboards::squares[to])
				score += _pIfo.eg.chainBaseAttackBonus;

			// If these are chain tips add a smaller bonus
			if (_ifo.pawnInfo.chainTips[c ^ 1] & bitboards::squares[to])
				score += _pIfo.eg.chainTipAttackBonus;
		}
		return score;
	}

}
