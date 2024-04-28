
#include "eval.h"

namespace Evaluation {

	namespace {
		/*Specialized preprocessing methods*/
		/*Specialized postprocessing methods*/
		/*Specialized initialize/utility methods*/
	}

	int Evaluation::eval_pawns(const position& p, const Searchthread& t) {

		// TODO: update fetch to return nullptr if nothing found (or true/false)
		// TODO: at the end of refactoring, adjust the tables to support 
		// the new approach to evaluation..for now *always* evaluate material
		//_dInfo.pe = t.pawnTable.fetch(p);
		//if (_dInfo.pe != nullptr) {
		//	// return score...
		//}

		// Fresh pawn structure/material for this position
		pawn_entry e;
		auto pScore = eval_pawns(p, e);

		// TODO: Store pawn entry in the pawn table

		return pScore;
	}


	int Evaluation::eval_pawns(const position& p, pawn_entry& e) {

		int score = 0;

		// Middlegame
		_dInfo.mg.pawn_scores.push_back(eval_pawn_island_mg(white, p, e) - eval_pawn_island_mg(black, p, e));

		// Endgame
		_dInfo.eg.pawn_scores.push_back(eval_pawn_island_eg(white, p, e) - eval_pawn_island_eg(black, p, e));

		// Post process scores
		return score;
	}

}

