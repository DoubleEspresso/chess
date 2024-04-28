
#include "eval.h"

namespace Evaluation {

	namespace {
		/*Specialized preprocessing methods*/
		/*Specialized postprocessing methods*/
		/*Specialized initialize/utility methods*/
	}

	void Evaluation::initalize(const position& p, const Searchthread& t) {

		// Note: these will *evaluate* pawn structure and material
		// if nothing is found in the pawn/material hash tables for this position (!)
		_dInfo.pe = t.pawnTable.fetch(p);
		_dInfo.me = t.materialTable.fetch(p);


	}


	int Evaluation::evaluate(const position& p, const Searchthread& t, const float& lazy_margin) {

		// Material (both middle game + endgame)
		auto matScore = eval_material(p, t);

		// Lazy eval if material score beyond the lazy margin..

		// Pawns (both middle game + endgame)
		auto pawnScore = eval_pawns(p, t);

		// ----- Evaluate static features of the position ---- //
		// Middlegame

		// Endgame
		_dInfo.eg.scores.push_back(0);


		// ----- Post-process scores and return ---- //

		return 0;
	};
}

