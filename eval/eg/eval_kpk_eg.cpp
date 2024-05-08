
#include "../eval.h"
#include "../eg/squares_eg.h"
#include "../mg/squares_mg.h"

namespace Evaluation {

	namespace {
		inline bool has_opposition(const Color& c, const position& p) {
			Square wks = p.king_square(white);
			Square bks = p.king_square(black);
			Color tmv = p.to_move();

			int cols = util::col_dist(wks, bks) - 1;
			int rows = util::row_dist(wks, bks) - 1;
			bool odd_rows = ((rows & 1) == 1);
			bool odd_cols = ((cols & 1) == 1);

			// distant opposition
			if (cols > 0 && rows > 0) {
				return (tmv != c && odd_rows && odd_cols);
			}
			// direct opposition
			else return (tmv != c && (odd_rows || odd_cols));
		}
	}


	int Evaluation::eval_kpk_eg(const Color& c, const position& p) {

		// - Passed pawn race evaluation
		// - Fence positions - draw detection
		// - Creating passed pawn
		// - King opposition
		// - Unstoppable passer eval
		// - Outside passed pawn

		int score = 0;
		auto them = Color(c ^ 1);
		auto enemyPawns = p.get_pieces<pawn>(them);

		// King opposition
		bool opposition = has_opposition(c, p);

		// Re-evaluate passed pawns
		U64 passed_pawns = _ifo.passed_pawns[c];
		if (passed_pawns != 0ULL) {
			while (passed_pawns) {
				int f = bits::pop_lsb(passed_pawns);
				score += eval_passed_kpk(c, p, Square(f), has_opposition);
			}
		}

		return score;
	}




	int Evaluation::eval_passed_kpk(const Color& c, const position& p, const Square& f, bool hasOpposition) {

		int score = 0;
		auto passers = _ifo.passed_pawns[c];


		Color them = Color(c ^ 1);

		Square ks = p.king_square(c);
		int row_ks = util::row(ks);
		int col_ks = util::col(ks);

		Square eks = p.king_square(them);
		int row_eks = util::row(eks);
		int col_eks = util::col(eks);

		int row = util::row(f);
		int col = util::col(f);

		Square in_front = Square(f + (c == white ? 8 : -8));

		U64 eks_bb = (bitboards::kmask[f] & bitboards::squares[eks]);
		U64 fks_bb = (bitboards::kmask[f] & bitboards::squares[ks]);

		bool e_control_next = eks_bb != 0ULL;
		bool f_control_next = fks_bb != 0ULL;

		bool f_king_infront = (c == white ? row_ks >= row : row_ks <= row);
		bool e_king_infront = (c == white ? row_eks > row : row_eks < row);

		// edge column draw
		if (col == Col::A || col == Col::H) {
			if (e_control_next)
				score -= _pIfo.eg.kpkEdgeColumnPenalty;
			if (col_eks == col && e_king_infront)
				score -= _pIfo.eg.kpkEdgeColumnPenalty;
		}

		// case 1. bad king position (enemy king blocking pawn)
		if (e_king_infront && !f_king_infront && e_control_next && !has_opposition)
			score -= _pIfo.eg.kpkBadKingPenalty;

		// case 2. we control front square and have opposition (winning)
		if (f_control_next && has_opposition)
			score += _pIfo.eg.kpkGoodKingBonus;

		int dist = (c == white ? 7 - row : row);

		// case 3. we are behind the pawn
		bool inside_pawn_box = util::col_dist(eks, f) <= dist;

		int fk_dist = std::max(util::col_dist(ks, f), util::row_dist(ks, f));
		int ek_dist = std::max(util::col_dist(eks, f), util::row_dist(eks, f));
		bool too_far = fk_dist >= ek_dist;
		if (too_far && !f_king_infront && inside_pawn_box)
			score -= _pIfo.eg.kpkKingBehindPenalty;

		// Connected passed bonus
		auto connectedPassed = (bitboards::neighbor_cols[util::col(f)] & passers) != 0ULL;
		if (connectedPassed)
			score += _pIfo.eg.passedConnectedBonus; // this is counted twice.

		// Outside passer
		if (util::col(f) == Col::A || util::col(f) == Col::H) {
			score += _pIfo.eg.passedOutsideBonus; // this is counted twice.
			if (!inside_pawn_box && dist <= 4)
				score += _pIfo.eg.passedOutsideNearQueenBonus; // good chance to queen this pawn
		}


		// bonus for being closer to queening
		if (f_control_next && has_opposition && dist >= 0 && dist <= 6)
			score += _pIfo.eg.kpkPassedDistScale[dist];

		return score;
	}

}
