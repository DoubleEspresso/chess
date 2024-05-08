
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

	void Evaluation::initalize_kpk_params() {

		// Manually adjusted
		//std::vector<double> kpkParams2 = {
		//	0.0, 8.22591, 12.8263, 19.383, 17.4264, 19.6681, 8.53642, 0, 0, 20.0491, 17.1824, 21.9946, 22.3184, 46.4112, 33.4118, 0, 32.5591, 51.988, 43.5087, 41.7642, 97.8158, 75.8682, 152.785, 0, 0, 15.6643, 26.0753, 0, 0, 31.8842, 43.3731, 40.1194, 15.697, 9.2567, 31.2159, 0, 37.3771, 73.144, 26.6721, 16.449, 12.5189, 57.0619, 21.7091, 23.9829, 11.5334, 30.9017, 0, 0, 0, 19.3213, 0, 21.2538, 51.8215, 30.8971, 9.708, 9.42774, 18.2398, 27.9313, 9.26061, 3.48194, 41.0865, 27.0431, 16.1891, 15.4709, 6.61765, 43.9648, 55.2416, 27.5195, 30.7363, 19.7943, 2.72402, 15.2211, 55.8872, 0, 0.207473, 114.717, 104.992, 77.3254, 62.6433, 28.5428, 33.6731, 23.2412
		//};

		// new
		std::vector<double> kpkParams2 = {
			0.000000,41.791188,41.745330,16.397518,5.066589,10.795588,26.334591,22.975139,41.059617,41.503527,21.496808,28.871342,72.959997,72.741334,151.979309,11.953970,8.802337,15.649477,17.016857,0.000000,0.000000,16.566748,43.732859,55.421646,25.555043,45.594998,30.821540,23.498755,44.392622,35.540375,13.944671,12.910889,3.682602,31.692582,37.767753,0.000000,19.571598,22.892870,23.219577,2.872691,23.499788,25.863745,19.875359,12.296516,18.516774,33.933973,15.137513,19.248224,19.305697,16.351789,0.000000,0.000000,20.631189,0.000000,17.310677,0.000000,8.581455,14.012670,56.428034,51.939009,20.788433,17.572497,4.232936,36.192004,63.743460,33.348060,35.366034,135.056756,95.480790,72.910961,83.293686,43.092789,24.260928,22.958237
		};


		std::vector<int> params;
		for (int i = 0; i < kpkParams2.size(); ++i)
			params.push_back(std::round(kpkParams2[i]));

		Initialize_KpK(params);

	}


	int Evaluation::evaluate(const position& p, const Searchthread& t, const float& lazy_margin) {

		// TODO: optimize this
		if (!isTuning)
			initalize_kpk_params();

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
		//_ifo.mg.scores.push_back(eval_knight_mg(white, p) - eval_knight_mg(black, p));		
		_ifo.mg.scores.push_back(eval_king_mg(white, p) - eval_king_mg(black, p));
		//_ifo.mg.scores.push_back((p.to_move() == white ? _pIfo.mg.tempo : -_pIfo.mg.tempo));
		
		auto mgScore = std::accumulate(_ifo.mg.scores.begin(), _ifo.mg.scores.end(), 0);


		// Endgame
		//_ifo.eg.scores.push_back(eval_king_eg(white, p) - eval_king_eg(black, p));
		//_ifo.eg.scores.push_back((p.to_move() == white ? _pIfo.eg.tempo : -_pIfo.eg.tempo));

		// Specialized endgame evaluations based on material eval
		// Note: these introduce discontinuities in search if they are not tuned perfectly
		// for example, if we give specialized bonuses for passed pawns in kpk endings,
		// the search may avoid queening to keep the passed pawn eval and avoid moving away from kpk ending type (!)
		// Instead of giving bonuses, consider draw detections that save search from useless effort
		// In this case, these methods would only adjust the score to 0 or leave the score untouched.

		if (_ifo.endgameType == EndgameType::KpK)
			_ifo.eg.scores.push_back(eval_kpk_eg(white, p) - eval_kpk_eg(black, p));

		auto egScore = std::accumulate(_ifo.eg.scores.begin(), _ifo.eg.scores.end(), 0);

		// ----- Post-process scores and return ---- //

		auto posScore = (int)std::round(
			mgScore * (1.0 - _ifo.egCoeff) + _ifo.egCoeff * egScore);

		// Accumulate all scores
		auto score = matScore + pawnScore + posScore;
		return (p.to_move() == white ? score : -score)/* + p.params.tempo*/;
	};
}

