
#include "eval.h"

namespace Evaluation {

	namespace {
		/*Specialized preprocessing methods*/
		/*Specialized postprocessing methods*/
		/*Specialized initialize/utility methods*/
	}


	int Evaluation::eval_material(const position& p, const Searchthread& t) {
		_dInfo.me = t.materialTable.fetch(p);
		if (_dInfo.me != nullptr) {
			// return score...
		}

		// Fresh evaluation of material for this position
		return eval_material(white, p) - eval_material(black, p);
	}


	int Evaluation::eval_material(const Color& stm, const position& p) {

		int score = 0;

		// Middlegame
		_dInfo.mg.mat_scores.push_back(eval_material_mg(white, p) - eval_material_mg(black, p));

		// Endgame
		_dInfo.eg.mat_scores.push_back(eval_material_eg(white, p) - eval_material_eg(black, p));

		// Post process scores
		return score;

	}
}

