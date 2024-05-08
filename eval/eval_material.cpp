
#include "eval.h"

namespace Evaluation {

	namespace {
		/*Specialized preprocessing methods*/
		/*Specialized postprocessing methods*/
		/*Specialized initialize/utility methods*/
	}


	int Evaluation::eval_material(const position& p, const Searchthread& t) {

		// TODO: at the end of refactoring, adjust the tables to support 
		// the new approach to evaluation..for now *always* evaluate material
		//_dInfo.me = t.materialTable.fetch(p);
		//if (_dInfo.me != nullptr) {
		//	// return score...
		//}

		// Fresh evaluation of material for this position
		material_entry e = { };
		auto mScore = eval_material(p, e);

		// TODO: Store material entry in the material table

		return mScore;
	}


	int Evaluation::eval_material(const position& p, material_entry& e) {

		int score = 0;


		// Middlegame
		_ifo.mg.mat_scores.push_back(eval_material_mg(white, p, e) - eval_material_mg(black, p, e));

		// Endgame
		_ifo.eg.mat_scores.push_back(eval_material_eg(white, p, e) - eval_material_eg(black, p, e));

		auto total = 0;
		for (auto& n : e.number)
			total += n;

		// encoding endgame type if applicable
		// see types.h for enumeration of different endgame types
		if (total == 0)
			_ifo.endgameType = EndgameType::KpK;


		// Compute the endgame interpolation coefficient
		// computed from piece count - excludes pawns
		// endgame_coeff of 0 --> piece count = 14
		// endgame_coeff of 1 --> piece count = 2
		// coeff = -1/12*total + 7/6
		_ifo.egCoeff = std::min(-0.083333f * total + 1.16667f, 1.0f);

		score = (int)std::round(
			_ifo.mg.mat_scores[0] * (1.0 - _ifo.egCoeff) + _ifo.egCoeff * _ifo.eg.mat_scores[0]);

		return score;

	}
}

