
#include <numeric>

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
		_ifo.pe = t.pawnTable.fetch(p);
		_ifo.me = t.materialTable.fetch(p);


	}


	int Evaluation::evaluate(const position& p, const Searchthread& t, const float& lazy_margin) {

		// Material (both middle game + endgame)
		auto matScore = eval_material(p, t);

		// TODO: Lazy eval if material score beyond the lazy margin..
		//if (lazy_margin > 0 && !ei.me->is_endgame() && abs(score) >= lazy_margin)
		//	return (p.to_move() == white ? score : -score) + p.params.tempo;


		_ifo.all_pieces		= p.all_pieces();
		_ifo.empty			= ~p.all_pieces();
		_ifo.pieces[white]	= p.get_pieces<white>();
		_ifo.pieces[black]	= p.get_pieces<black>();

		// Pawns (both middle game + endgame)
		auto pawnScore = eval_pawns(p, t);

		// ----- Evaluate static features of the position ---- //
		// Middlegame
		auto mgScore = 0;
		//_ifo.mg.scores.push_back(eval_knight_mg(white, p) - eval_knight_mg(black, p));		
		
		mgScore = std::accumulate(_ifo.mg.scores.begin(), _ifo.mg.scores.end(), 0);

		// Endgame
		auto egScore = 0;


		// ----- Post-process scores and return ---- //
		auto posScore = (int)std::round(
			mgScore * (1.0 - _ifo.egCoeff) + _ifo.egCoeff * egScore);

		// Accumulate all scores
		auto score = matScore + pawnScore + posScore;
		return (p.to_move() == white ? score : -score)/* + p.params.tempo*/;
	};
}

