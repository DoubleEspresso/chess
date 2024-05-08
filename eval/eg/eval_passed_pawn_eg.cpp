
#include "../eval.h"

namespace Evaluation {


	int Evaluation::eval_passed_pawn_eg(const Color& c, const position& p) {

		// Bonus for protected passed pawns [x]
		// Bonus for 'outside' passed pawns [x]
		// King tropism (distance to our king, distance to their king) [x]
		// Distance to promotion [x]
		// Stoppable or unstoppable (king square/blockaders)
		// Rooks behind our passed pawns bonus [x]
		// Enemy rooks behind our passed pawns penalty [x]
		// Front square control [x]

		float score = 0;
		U64 passers = _ifo.passed_pawns[c];
		U64 attacks = _ifo.pawn_attacks[c];
		if (passers == 0ULL) {
			return score;
		}

		auto rooks = p.get_pieces<rook>(c);
		auto erooks = p.get_pieces<rook>(Color(c ^ 1));
		auto ks = p.king_square(c);
		auto eks = p.king_square(Color(c ^ 1));

		while (passers) {
			Square f = Square(bits::pop_lsb(passers));
			int fromQueen = (c == white ? 7 - util::row(f) : util::row(f));
			score += _pIfo.eg.passedPawnBonus;
			Square front = (c == white ? Square(f + 8) : Square(f - 8));
			auto column = util::col(f);

			// Front square unblocked bonus
			if (p.piece_on(front) == Piece::no_piece)
				score += _pIfo.eg.passedUnblockedBonus;

			// Connected passed bonus
			auto connectedPassed = (bitboards::neighbor_cols[util::col(f)] & passers) != 0ULL;
			if (connectedPassed)
				score += _pIfo.eg.passedConnectedBonus; // this is counted twice.

			// Protected passed bonus
			auto defended = (attacks & bitboards::squares[f]) != 0ULL;
			if (defended)
				score += _pIfo.eg.passedDefendedBonus;

			// Outside passed pawn bonus
			if (column == Col::A || column == Col::H)
				score += _pIfo.eg.passedOutsideBonusAH;
			if (column == Col::B || column == Col::G)
				score += _pIfo.eg.passedOutsideBonusBG;

			// King distance (bonus for closer to our king)
			int dist = std::max(util::row_dist(f, ks), util::col_dist(f, ks));
			score += _pIfo.eg.passedKingDistBonus[dist];

			// Penalty for being closer to enemy king
			dist = std::max(util::row_dist(f, eks), util::col_dist(f, eks));
			score -= _pIfo.eg.passedEKingDistPenalty[dist];

			// Rook support bonus
			if (rooks)
			{
				while (rooks)
				{
					auto rf = Square(bits::pop_lsb(rooks));
					if (util::col(rf) == util::col(f))
					{
						auto rowDiff = util::row(rf) - util::row(f);
						auto isBehind = (c == white ? rowDiff < 0 : rowDiff > 0);
						auto supports = ((bitboards::between[rf][f] & p.all_pieces()) ^ (bitboards::squares[rf] | bitboards::squares[f])) == 0ULL;
						if (isBehind)
							score += _pIfo.eg.passedRookBonus;
						if (isBehind && supports)
							score += _pIfo.eg.passedRookBehindBonus;
					}
				}
			}

			// Enemy rook behind pawn penalty
			if (erooks) {
				while (erooks)
				{
					auto rf = Square(bits::pop_lsb(erooks));
					if (util::col(rf) == util::col(f))
					{
						auto rowDiff = util::row(rf) - util::row(f);
						auto isBehind = (c == white ? rowDiff < 0 : rowDiff > 0);
						auto defends = ((bitboards::between[rf][f] & p.all_pieces()) ^ (bitboards::squares[rf] | bitboards::squares[f])) == 0ULL;
						if (isBehind)
							score -= _pIfo.eg.passedERookPenalty;
						if (isBehind && defends)
							score -= _pIfo.eg.passedERookBehindPenalty;
					}
				}
			}

			// Control front square
			U64 our_attackers = 0ULL;
			U64 their_attackers = 0ULL;
			bool frontSqControl = false;
			if (util::on_board(front))
			{
				our_attackers = p.attackers_of2(front, c);
				their_attackers = p.attackers_of2(front, Color(c ^ 1));
			}
			if (our_attackers != 0ULL) {
				if (their_attackers != 0ULL) {
					auto fCount = bits::count(our_attackers);
					auto eCount = bits::count(their_attackers);
					if (fCount >= eCount) {
						score += _pIfo.eg.passedPawnControlBonus;
						frontSqControl = true;
					}
				}
				else
					score += _pIfo.eg.passedPawnControlBonus;
			}

			// Is the enemy king inside the passed pawn square
			// Note: this is imperfect, since the king's path can still be obstructed
			auto square = bitboards::passpawn_square[c][f];
			if (bitboards::squares[f] & square) {
				score -= _pIfo.eg.passedEKingWithinSqPenalty;
			}

			// Penalty if enemy king blocks promotion path
			auto pp = bitboards::passpawn_mask[c][f];
			if (bitboards::squares[eks] & pp & bitboards::col[util::col(f)]) {
				score -= _pIfo.eg.passedEKingBlockerPenalty;
			}

			// If we have control and not blocked by enemy king
			// Then add this distance bonus
			if (frontSqControl && fromQueen)
				score += _pIfo.eg.passedDistBonus[fromQueen - 1];
		}

		return score;
	}

}
