
#include "eval.h"

namespace Evaluation {

	namespace {
		/*Specialized preprocessing methods*/
		/*Specialized postprocessing methods*/
		/*Specialized initialize/utility methods*/
	}

	int Evaluation::eval_pawns(const position& p, const Searchthread& t) {

		// TODO: update fetch to return nullptr if nothing found (or true/false)
		_dInfo.pe = t.pawnTable.fetch(p);
		if (_dInfo.pe != nullptr) {
			// return score...
		}

		// Fresh evaluation of pawn structure for this position
		return eval_pawns(white, p) - eval_pawns(black, p);
	}


	int Evaluation::eval_pawns(const Color& stm, const position& p) {

		int score = 0;

		// Middlegame
		_dInfo.mg.pawn_scores.push_back(eval_pawn_island_mg(white, p) - eval_pawn_island_mg(black, p));

		// Endgame
		_dInfo.eg.pawn_scores.push_back(eval_pawn_island_eg(white, p) - eval_pawn_island_eg(black, p));

		// Post process scores
		return score;
	}

}

