
#include "../eval.h"

namespace Evaluation {


	int Evaluation::eval_doubled_pawn_eg(const Color& c, const position& p) {
		int score = 0;
		return score;
		// Simple evaluation - we want fewer pawn islands than our opponent.
		// Ideally we have at most 2.
		const auto& doubled = _ifo.doubled_pawns[c];
		if (!doubled)
			return score;

		// How many pawns are doubled..
		auto nDoubled = bits::count(doubled);
		auto N = std::max(0, std::min((int)nDoubled, (int)_pIfo.eg.doubledPenalty.size() - 1));
		score -= _pIfo.eg.doubledPenalty[N];

		// Additionaly penalties for being on an opponent's semi-open file
		// only if rooks on the board
		auto rooks = p.get_pieces<rook>(Color(c ^ 1));
		const auto& semiOpen = _ifo.semiOpen_files[c ^ 1];
		const auto openDoubled = semiOpen & doubled;
		if (rooks && openDoubled) {
			nDoubled = bits::count(openDoubled);
			auto N = std::max(0, std::min((int)openDoubled, (int)_pIfo.eg.doubledOpen.size() - 1));
			score -= _pIfo.eg.doubledOpen[N];
		}

		// Give a small bonus if any of these doubled pawns are defended
		//const auto defended = _ifo.pawnInfo.defended[c] & doubled;
		//if (defended)
		//	score += _pIfo.eg.doubledDefendedBonus;

		return score;
	}

}
