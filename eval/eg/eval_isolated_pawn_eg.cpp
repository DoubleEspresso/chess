
#include "../eval.h"

namespace Evaluation {


	int Evaluation::eval_isolated_pawn_eg(const Color& c, const position& p) {
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
		const auto& eks = p.king_square(Color(c ^ 1));
		const auto& ks = p.king_square(Color(c ^ 1));

		while (isolated) {

			auto s = bits::pop_lsb(isolated);

			// Simple penalty if the isolated pawn is not central
			// Here all isolated pawns are considered weak (very weak).
			// Note: that an outside passed pawn is generally a strength.
			// Make sure the passed pawn eval overcomes this penalty (!)
			score -= _pIfo.eg.isolatedPenalty;


			// In kpk endings, isolated pawns nearer the enemy king
			// are problems
			if (_ifo.endgameType == EndgameType::KpK) {
				int fk_dist = std::max(util::col_dist(eks, s), util::row_dist(ks, s));
				int ek_dist = std::max(util::col_dist(eks, s), util::row_dist(eks, s));
				if (fk_dist > ek_dist)
					score -= _pIfo.eg.isolatedKingDistPenalty;
			}


			// 4. Is this pawn blockaded - do we control the 'front sq'
			auto front = (c == white ? s + 8 : s - 8);
			auto eAttks = p.attackers_of2(Square(front), Color(c ^ 1));
			auto fAttks = p.attackers_of2(Square(front), Color(c));
			if (eAttks) {
				auto eCount = bits::count(eAttks);
				auto fCount = bits::count(fAttks);
				if (eCount > fCount)
					score -= _pIfo.eg.isolatedControlPenalty;
			}
		}

		return score;
	}

}
