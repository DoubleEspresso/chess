
#include "../eval.h"

namespace Evaluation {

	namespace {
		/*Specialized debug methods*/
		/*Specialized square scores*/
		/*Specialized utility methods*/
		const std::vector<Piece> pieces{ knight, bishop, rook, queen }; // pawns handled in pawns.cpp

	}

	int Evaluation::eval_material_mg(const Color& c, const position& p) {
		int score = 0;

		material_entry e;

		for (const auto& piece : pieces) {
			int n = p.number_of(c, piece);
			e.number[piece] += n;
			score += n * _pInfo.eg.material_vals[piece];
		}

		return score;
	}

}
