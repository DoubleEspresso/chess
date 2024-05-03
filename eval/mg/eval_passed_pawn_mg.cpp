
#include "../eval.h"

namespace Evaluation {


	int Evaluation::eval_passed_pawn_mg(const Color& c, const position& p) {
		float score = 0;
		U64 passers = _ifo.passed_pawns[c];
		U64 attacks = _ifo.pawn_attacks[c];
		if (passers == 0ULL) {
			return score;
		}

		while (passers) {
			Square f = Square(bits::pop_lsb(passers));
			int fromQueen = (c == white ? 7 - util::row(f) : util::row(f));
			score += _pIfo.mg.passedPawnBonus;
			Square front = (c == white ? Square(f + 8) : Square(f - 8));

			// Front square unblocked bonus
			if (p.piece_on(front) == Piece::no_piece)
				score += _pIfo.mg.passedUnblockedBonus;

			// Connected passed bonus
			auto connectedPassed = (bitboards::neighbor_cols[util::col(f)] & passers) != 0ULL;
			if (connectedPassed)
				score += _pIfo.mg.passedConnectedBonus; // this is counted twice.

			// Protected passed bonus
			auto defended = (attacks & bitboards::squares[f]) != 0ULL;
			if (defended)
				score += _pIfo.mg.passedDefendedBonus; // this is counted twice.

			// Distance to promotion bonus
			score += (
				fromQueen == 3 ? _pIfo.mg.passedDistBonus[fromQueen - 1] :
				fromQueen == 2 ? _pIfo.mg.passedDistBonus[fromQueen - 1] :
				fromQueen == 1 ? _pIfo.mg.passedDistBonus[fromQueen - 1] : 0);
		}
		return score;
	}

}
