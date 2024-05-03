
#include "../eval.h"

namespace Evaluation {


	int Evaluation::eval_passed_pawn_eg(const Color& c, const position& p) {
		int score = 0;

		// Bonus for protected passed pawns
		// Bonus for 'outside' passed pawns
		// King tropism (distance to king)
		// Distance to promotion
		// Stoppable or unstoppable (king square/blockaders)
		// Rooks behind our passed pawns, or behind their passed pawns (part of rook eval?)
		// Candidate passed pawn detection (part of pawn majority eval?)
		// Front square control

		// Bonus for rooks behind passed pawns (endgame only?)
		//auto rooks = (c == white ? p.get_pieces<white, rook>() : p.get_pieces<black, rook>());
		//if (rooks)
		//{
		//	while (rooks)
		//	{


		//		auto rf = Square(bits::pop_lsb(rooks));
		//		if (util::col(rf) == util::col(f))
		//		{
		//			auto rowDiff = util::row(rf) - util::row(f);
		//			auto isBehind = (c == white ? rowDiff < 0 : rowDiff > 0);
		//			auto supports = ((bitboards::between[rf][f] & p.all_pieces()) ^ (bitboards::squares[rf] | bitboards::squares[f])) == 0ULL;
		//			if (isBehind)
		//				score += 1;
		//			if (isBehind && supports)
		//			{
		//				crudeControl += 1;
		//				score += 30;
		//			}
		//		}
		//	}
		//}

			// Do we have 'crude' control over the square in front of the pawn
		//U64 our_attackers = 0ULL;
		//U64 their_attackers = 0ULL;
		//if (util::on_board(front))
		//{
		//	our_attackers = p.attackers_of2(front, c);
		//	their_attackers = p.attackers_of2(front, Color(c ^ 1));
		//}
		//if (our_attackers != 0ULL) {
		//	if (their_attackers != 0ULL) {
		//		auto fCount = bits::count(our_attackers);
		//		auto eCount = bits::count(their_attackers);
		//		if (fCount >= eCount)
		//			score += _pIfo.mg.passedPawnControlBonus;
		//	}
		//	else
		//		score += _pIfo.mg.passedPawnControlBonus;
		//}

		return score;
	}

}
