
#include "../eval.h"

namespace Evaluation {

	int Evaluation::eval_pawn_island_eg(const Color& c, const position& p) {
		int score = 0;

		// Simple evaluation - we want fewer pawn islands than our opponent.
		// Ideally we have at most 2.
		const auto& islands = (c == white ? _ifo.pawnInfo.wPawnIslands : _ifo.pawnInfo.bPawnIslands);
		auto N = std::max(0, std::min((int)islands.size(), (int)_pIfo.eg.islandPenalty.size() - 1));
		score -= _pIfo.eg.islandPenalty[N];

		return score;
	}

}
