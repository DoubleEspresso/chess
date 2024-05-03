
#include "../eval.h"

namespace Evaluation {


	int Evaluation::eval_pawn_majority_eg(const Color& c, const position& p) {
		int score = 0;

		if (_ifo.pawnInfo.kPawnMajorities[c])
			score += _pIfo.eg.kMajorityBonus;

		if (_ifo.pawnInfo.qPawnMajorities[c])
			score += _pIfo.eg.qMajorityBonus;

		return score;
	}

}
