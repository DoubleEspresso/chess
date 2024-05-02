
#include "../eval.h"

namespace Evaluation {


	int Evaluation::eval_isolated_pawn_mg(const Color& c, const position& p) {
		int score = 0;

		// Things to check to ensure this isolated pawn is not weak
		//  - control of open semi-open files (rooks placed on our semi-open files)
		//  - bishop pair (pointed at king)
		//  - control of outposts (knight posted/attacking isolani attack pts)
		//  - not blockaded, not attacked ?
		//  - is a central pawn?
		//  - is passed
		auto isolated = _ifo.isolated_pawns[c];
		if (!isolated)
			return score;

		const auto& pawns = p.get_pieces<pawn>(c);
		const auto& rooks = p.get_pieces<rook>(c);
		const auto& queens = p.get_pieces<queen>(c);
		const auto& bishops = p.get_pieces<bishop>(c);
		const auto& knights = p.get_pieces<knight>(c);

		while (isolated) {

			auto s = bits::pop_lsb(isolated);

			// Simple penalty if the isolated pawn is not central
			if (util::col(s) != Col::D && util::col(s) != Col::E) {
				score -= _pIfo.mg.isolatedPenalty;
				continue;
			}

			// Reject cases where there are multiple isolated pawns on this file
			auto colPawns = bitboards::col[s] & pawns;
			if (bits::more_than_one(colPawns))
				continue; // Penalties already accrued from doubled pawn eval.

			// 0. Flat bonus for having the bishop pair (better attacking potential)
			if (bishops && bits::more_than_one(bishops))
				score += _pIfo.mg.isolated2BishopsBonus;

			// 1. Do we have semi-open files - note: these items are naturally
			// accounted for in mobility scores for each piece (!)
			const auto& semiOpen = _ifo.semiOpen_files[c] & bitboards::neighbor_cols[s];
			auto rookOnFile = semiOpen & rooks;
			if (rookOnFile)
				score += _pIfo.mg.isoFileBonus[rook];
			auto queenOnFile = semiOpen & queens;
			if (queenOnFile)
				score += _pIfo.mg.isoFileBonus[queen];

			// 2. Do we have a knight positioned on the isolani outpost
			// (Not sure this is really a valid 'bonus' for isolated pawns)
			// Also this is likely accounted for in the sq scores/knight eval.
			auto outPosts = bitboards::pattks[c][s];
			if (knights & outPosts)
				score += _pIfo.mg.isolatedKnightPostBonus;

			// 3. Passed pawn penalty - rational is that this pawn is now easier to attack
			auto passed = _ifo.passed_pawns[c] & bitboards::squares[s];
			if (passed)
				score -= _pIfo.mg.isolatedPassedPenalty;

			// 4. Is this pawn blockaded - do we control the 'front sq'
			auto front = (c == white ? s + 8 : s - 8);
			auto eAttks = p.attackers_of2(Square(front), Color(c ^ 1));
			auto fAttks = p.attackers_of2(Square(front), Color(c));
			if (eAttks) {
				auto eCount = bits::count(eAttks);
				auto fCount = bits::count(fAttks);
				if (eCount > fCount)
					score -= _pIfo.mg.isolatedControlPenalty;
			}
		}

		return score;
	}

}
