
#include "../eval.h"

namespace Evaluation {

	namespace {
		/*Specialized debug methods*/
		/*Specialized square scores*/
		/*Specialized utility methods*/
		const std::vector<Piece> pieces{ knight, bishop, rook, queen }; // pawns handled in pawns.cpp
	}

	int Evaluation::eval_material_eg(const Color& c, const position& p, material_entry& e) {
		int score = 0;
		auto pieceNumber = std::vector< std::vector<int> >{
			{ 0, 0, 0, 0, 0 }, // p, n, b, r, q
			{ 0, 0, 0, 0, 0 } };// p, n, b, r, q

		int total = 0;
		for (const auto& piece : pieces) {
			int n = p.number_of(c, piece);
			// Note: the numbers are *already* tracked in the mg evaluation, do not double-count them(!)
			pieceNumber[c][piece] += n;
			score += n * _pIfo.eg.material_vals[piece];
			total += n;
		}


		// Add more as necessary...

		return score;
	}

}
