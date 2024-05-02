

#ifndef SQUARES_EG_H
#define SQUARES_EG_H


#include "../../types.h"
#include "../../utils.h"


namespace Evaluation {


	namespace {
		static const int eg_scores[6][64] =
		{
			{
				// pawns
				-10, -10, -10, -10,  -10,  -10,  -10, -10,
				-10, -10, -10, -10,  -10,  -10,  -10, -10,
				  0,   0,   0,   8,    8,    0,    0,   0,
				  4,   6,   8,   10,  10,    8,    6,   4,
				  6,   8,   10,  10,  10,   10,    8,   6,
				 10,  10,   10,  10,  10,   10,   10,  10,
				 10,  10,   10,  10,  10,   10,   10,  10,
				 10,  10,   10,  10,  10,   10,   10,  10,
			},

			{
				// knights
				-10, -10, -10,  -10,  -10,  -10,  -10,  -10,
				-10,  -8,  -8,   -4,   -4,   -8,   -8,  -10,
				-10,  -2,   6,    6,    6,    6,   -2,  -10,
				-10,  -2,   8,    8,    8,    8,   -2,  -10,
				-10,  -2,   10,  10,   10,   10,   -2,  -10,
				-10,  -2,   6,   10,   10,    6,   -2,  -10,
				-10,  -8,  -8,   -4,   -4,   -8,   -8,  -10,
				-10, -10, -10,  -10,  -10,  -10,  -10,  -10,
			},

			{
				// bishops
				 -5, -10, -10,  -10,  -10,  -10,  -10,   -5,
				-10,   0,  -8,   -4,   -4,   -8,   -8,  -10,
				-10,  -2,   2,    6,    6,    2,   -2,  -10,
				-10,  -2,   6,   10,   10,    6,   -2,  -10,
				-10,  -2,   8,   10,   10,    8,   -2,  -10,
				-10,  -2,   0,    0,    0,    0,   -2,  -10,
				-10, -10, -10,  -10,  -10,  -10,  -10,  -10,
				-10, -10, -10,  -10,  -10,  -10,  -10,  -10,
			},

			{
				// rooks
				-4,   2,   6,   10,   10,   6,    2,   -4,
				-4,   2,   6,   10,   10,   6,    2,   -4,
				-4,   2,   6,   10,   10,   6,    2,   -4,
				-4,   2,   6,   10,   10,   6,    2,   -4,
				-4,   2,   6,   10,   10,   6,    2,   -4,
				-4,   2,   6,   10,   10,   6,    2,   -4,
				 8,   8,   8,   10,   10,   8,    8,    8,
				-4,   2,   6,   10,   10,   6,    2,   -4,
			},

			{
				// queens
				-10, -10, -10,  -10,  -10,  -10,  -10,  -10,
				-10,  -4,  -4,   -4,   -4,   -4,   -4,  -10,
				-10,  -2,   4,    6,    6,    4,   -4,  -10,
				-10,  -2,   8,   10,   10,    8,   -2,  -10,
				-10,  -2,   8,   10,   10,    8,   -2,  -10,
				-10,  -2,   6,   10,   10,    6,   -2,  -10,
				-10,  -2,   2,    6,    6,    2,   -4,  -10,
				-10, -10, -10,  -10,  -10,  -10,  -10,  -10,
			},

			{
				// kings
				-10, -10, -10,  -10,  -10,  -10,  -10,  -10,
				-4,   2,   4,    6,    6,   4,    2,   -4,
				-4,   2,   6,   10,   10,   6,    2,   -4,
				-4,   3,   7,   10,   10,   7,    3,   -4,
				-4,   3,   8,   10,   10,   8,    3,   -4,
				-4,   4,   8,   10,   10,   8,    4,   -4,
				-4,   8,   8,   10,   10,   8,    8,   -4,
				-4,   8,   8,   10,   10,   8,    8,   -4,
			}
		};



		template<Color c> inline float eg_square_score(const Piece& p, const Square& s);


		template<> float eg_square_score<black>(const Piece& p, const Square& s) {
			return 6 * eg_scores[p][56 - 8 * util::row(s) + util::col(s)];
		}


		template<> float eg_square_score<white>(const Piece& p, const Square& s) {
			return 6 * eg_scores[p][s];
		}
	}

#endif


}