
#include "../eval.h"

namespace Evaluation {


	int Evaluation::eval_pawn_majority_mg(const Color& c, const position& p) {
		int score = 0;

		if (_ifo.pawnInfo.kPawnMajorities[c])
			score += _pIfo.mg.kMajorityBonus;

		if (_ifo.pawnInfo.qPawnMajorities[c])
			score += _pIfo.mg.qMajorityBonus;

		return score;
	}

}
