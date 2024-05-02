#include <numeric>

#include "eval.h"

namespace Evaluation {

	namespace {

		inline void find_open_files(const position& p, EData& e) {
			auto pawns = p.get_pieces<pawn>(white) | p.get_pieces<pawn>(black);
			U64 open = 0ULL;
			for (int c = 0; c < 8; ++c) {
				auto hasPawns = bitboards::col[c] & pawns;
				if (!hasPawns)
					open |= bitboards::col[c];
			}
			e.open_files = open;
		}

		inline void find_semiopen_files(const Color& c, const position& p, EData& e) {
			auto pawns = p.get_pieces<pawn>(c);
			U64 open = 0ULL;
			for (int c = 0; c < 8; ++c) {
				auto hasPawns = bitboards::col[c] & pawns;
				if (!hasPawns)
					open |= bitboards::col[c];
			}
			e.semiOpen_files[c] = open;
		}

		inline int find_pawn_islands(const Color& c, const position& p, EData& e) {
			auto pawns = p.get_pieces<pawn>(c);
			std::vector<U64> pawnIslands;
			bool isIsland = false;
			U64 island = 0ULL;
			for (int cIdx = 0; cIdx < 8; ++cIdx) {
				auto hasPawns = bitboards::col[cIdx] & pawns;
				if (hasPawns) {
					isIsland = true;
					island |= hasPawns;
				}
				else if (!hasPawns && isIsland) {
					isIsland = false;
					pawnIslands.push_back(island);
					island = 0ULL;
				}
				if (hasPawns && cIdx == 7)
					pawnIslands.push_back(island);
			}
			if (c == white)
				e.pawnInfo.wPawnIslands = pawnIslands;
			else
				e.pawnInfo.bPawnIslands = pawnIslands;

			return pawnIslands.size();
		}

		inline void find_doubled_pawns(const Color& c, const position& p, EData& e) {
			auto pawns = p.get_pieces<pawn>(c);
			U64 doubled = 0ULL;
			for (int cIdx = 0; cIdx < 8; ++cIdx) {
				auto filePawns = bitboards::col[cIdx] & pawns;
				if (filePawns && bits::more_than_one(filePawns))
					doubled |= filePawns;
			}
			e.doubled_pawns[c] = doubled;
		}

		inline void find_isolated_pawns(const Color& c, const position& p, EData& e) {
			auto pawns = p.get_pieces<pawn>(c);
			U64 isolated = 0ULL;
			for (int cIdx = 0; cIdx < 8; ++cIdx) {
				auto neighbors = bitboards::neighbor_cols[cIdx] & pawns;
				if (!neighbors)
					isolated |= bitboards::col[cIdx] & pawns;
			}
			e.isolated_pawns[c] = isolated;
		}

		inline void find_backward_pawns(const Color& c, const position& p, EData& e) {
			auto pawns = p.get_pieces<pawn>(c);
			U64 backward = 0ULL;
			for (int cIdx = 0; cIdx < 8; ++cIdx) {
				auto neighbors = bitboards::neighbor_cols[cIdx] & pawns;
				auto thisCol = bitboards::col[cIdx] & pawns;
				if (bits::more_than_one(neighbors) && thisCol) {
					auto s1 = bits::pop_lsb(neighbors);
					auto s2 = bits::pop_lsb(neighbors);
					auto s3 = bits::pop_lsb(thisCol);
					if (c == white && util::row(s3) < util::row(s1) && util::row(s3) < util::row(s2))
						backward |= bitboards::squares[s3];
					else if (c == black && util::row(s3) > util::row(s1) && util::row(s3) > util::row(s2))
						backward |= bitboards::squares[s3];
				}
				else if (neighbors && thisCol) { // just 1 neighbor (hanging pawns)
					auto s1 = bits::pop_lsb(neighbors);
					auto s3 = bits::pop_lsb(thisCol);
					if (c == white && util::row(s3) < util::row(s1))
						backward |= bitboards::squares[s3];
					else if (c == black && util::row(s3) > util::row(s1))
						backward |= bitboards::squares[s3];
				}
			}
			e.backward_pawns[c] = backward;
		}

		inline void find_majorities(const position& p, EData& e) {
			auto wPawns = p.get_pieces<pawn>(white);
			auto bPawns = p.get_pieces<pawn>(black);

			auto qSide = bitboards::col[Col::A] | bitboards::col[Col::B] |
				bitboards::col[Col::C] | bitboards::col[Col::D];
			auto kSide = bitboards::col[Col::E] | bitboards::col[Col::F] |
				bitboards::col[Col::G] | bitboards::col[Col::H];

			auto wQside = wPawns & qSide;
			auto wKside = wPawns & kSide;
			auto bQside = bPawns & qSide;
			auto bKside = bPawns & kSide;

			if (bits::count(wQside ) > bits::count(bQside))
				e.pawnInfo.qPawnMajorities[white] = wPawns & qSide;
			else if (bits::count(wQside) < bits::count(bQside))
				e.pawnInfo.qPawnMajorities[black] = bPawns & qSide;

			if (bits::count(wKside) > bits::count(bKside))
				e.pawnInfo.kPawnMajorities[white] = wPawns & kSide;
			else if (bits::count(wKside) < bits::count(bKside))
				e.pawnInfo.kPawnMajorities[black] = bPawns & kSide;
		}

		inline void find_colored_pawns(const position& p, EData& e) {
			auto wPawns = p.get_pieces<pawn>(white);
			e.pawnInfo.lightSqPawns[white] = bitboards::colored_sqs[white] & wPawns;
			e.pawnInfo.darkSqPawns[white] = bitboards::colored_sqs[black] & wPawns;

			auto bPawns = p.get_pieces<pawn>(black);
			e.pawnInfo.lightSqPawns[black] = bitboards::colored_sqs[white] & bPawns;
			e.pawnInfo.darkSqPawns[black] = bitboards::colored_sqs[black] & bPawns;
		}

		// Note this method also finds defended/undefended pawns
		inline void find_pawn_attacks(const Color& c, const position& p, EData& e) {
			U64 pawns = p.get_pieces<pawn>(c);
			U64 attacks = 0ULL;
			U64 undefended = 0ULL;

			Square* sqs = (c == white ? p.squares_of<white, pawn>() : p.squares_of<black, pawn>());
			for (auto s = *sqs; s != no_square; s = *++sqs) {
				attacks |= bitboards::pattks[c][s];
			}
			e.pawn_attacks[c] = attacks;
			e.pawnInfo.defended[c] = attacks & pawns;
			e.pawnInfo.undefended[c] = pawns & (~e.pawnInfo.defended[c]);
		}

		inline void find_pawn_bases(const Color& c, const position& p, EData& e) {
			// Pawns undefended by pawns - members of islands are 'bases' of chains
			auto islands = (c == white ? e.pawnInfo.wPawnIslands : e.pawnInfo.bPawnIslands);
			auto undefended = e.pawnInfo.undefended[c];
			U64 chainBases = 0ULL;
			for (const auto& chain : islands) {
				chainBases |= (chain & undefended);
			}
			e.pawnInfo.chainBases[c] = chainBases;
		}

		inline void find_chain_tips(const Color& c, const position& p, EData& e) {
			auto islands = (c == white ? e.pawnInfo.wPawnIslands : e.pawnInfo.bPawnIslands);
			auto undefended = e.pawnInfo.undefended[c];
			U64 chainTips = 0ULL;
			for (const auto& chain : islands) {
				if (c == white) {
					for (int r = 7; r >= 0; --r) {
						if (chain & bitboards::row[r]) {
							chainTips |= (chain & bitboards::row[r]);
							break;
						}
					}
				}
				else {
					for (int r = 0; r < 8; ++r) {
						if (chain & bitboards::row[r]) {
							chainTips |= (chain & bitboards::row[r]);
							break;
						}
					}
				}
			}
			e.pawnInfo.chainTips[c] = chainTips;
		}

		inline void find_passed_pawns(const Color& c, const position& p, EData& e) {
			U64 pawns = p.get_pieces<pawn>(c); 
			U64 epawns = p.get_pieces<pawn>(Color(c ^ 1));
			U64 passed = 0ULL;

			Square* sqs = (c == white ? p.squares_of<white, pawn>() : p.squares_of<black, pawn>());
			for (auto s = *sqs; s != no_square; s = *++sqs) {
				U64 mask = bitboards::passpawn_mask[c][s] & epawns;
				if (!mask)
					passed |= bitboards::squares[s];
			}
			e.passed_pawns[c] = passed;
		}

		inline void find_king_shelter(const Color& c, const position& p, EData& e) {
			U64 pawns = p.get_pieces<pawn>(c);
			auto ks = p.king_square(c);
			e.kingShelter[c] = pawns & bitboards::kzone[ks];
		}
	}



	int Evaluation::eval_pawns(const position& p, const Searchthread& t) {

		// TODO: update fetch to return nullptr if nothing found (or true/false)
		// TODO: at the end of refactoring, adjust the tables to support 
		// the new approach to evaluation..for now *always* evaluate material
		//_dInfo.pe = t.pawnTable.fetch(p);
		//if (_dInfo.pe != nullptr) {
		//	// return score...
		//}

		// Fresh pawn structure/material for this position
		pawn_entry e;
		auto pScore = eval_pawns(p, e);

		// TODO: Store pawn entry in the pawn table

		return pScore;
	}


	int Evaluation::eval_pawns(const position& p, pawn_entry& e) {

		int score = 0;

		// Preprocessing
		find_open_files(p, _ifo);
		find_semiopen_files(white, p, _ifo);
		find_semiopen_files(black, p, _ifo);
		find_pawn_islands(white, p, _ifo);
		find_pawn_islands(black, p, _ifo);
		find_doubled_pawns(white, p, _ifo);
		find_doubled_pawns(black, p, _ifo);
		find_isolated_pawns(white, p, _ifo);
		find_isolated_pawns(black, p, _ifo);
		find_backward_pawns(white, p, _ifo);
		find_backward_pawns(black, p, _ifo);
		find_majorities(p, _ifo);
		find_colored_pawns(p, _ifo);
		find_pawn_attacks(white, p, _ifo);
		find_pawn_attacks(black, p, _ifo);
		find_pawn_bases(white, p, _ifo);
		find_pawn_bases(black, p, _ifo);
		find_chain_tips(white, p, _ifo);
		find_chain_tips(black, p, _ifo);
		find_passed_pawns(white, p, _ifo);
		find_passed_pawns(black, p, _ifo);
		find_king_shelter(white, p, _ifo);
		find_king_shelter(black, p, _ifo);
		// Center specific
		// Pawn storms


		if (_trace) {
			std::cout << "\n  === open files === " << std::endl;
			bits::print(_ifo.open_files);

			std::cout << "\n  === semiopen files white === " << std::endl;
			bits::print(_ifo.semiOpen_files[white]);

			std::cout << "\n  === semiopen files black === " << std::endl;
			bits::print(_ifo.semiOpen_files[black]);

			std::cout << "\n  ===  white pawn islands === " << std::endl;
			for(const auto& i : _ifo.pawnInfo.wPawnIslands)
				bits::print(i);
			std::cout << "\n  ===  black pawn islands === " << std::endl;
			for (const auto& i : _ifo.pawnInfo.bPawnIslands)
				bits::print(i);

			std::cout << "\n  ===  white pawn majorities === " << std::endl;
			bits::print(_ifo.pawnInfo.qPawnMajorities[white]);
			bits::print(_ifo.pawnInfo.kPawnMajorities[white]);

			std::cout << "\n  ===  black pawn majorities === " << std::endl;
			bits::print(_ifo.pawnInfo.qPawnMajorities[black]);
			bits::print(_ifo.pawnInfo.kPawnMajorities[black]);

			std::cout << "\n  ===  white doubled === " << std::endl;
			bits::print(_ifo.doubled_pawns[white]);

			std::cout << "\n  ===  black doubled === " << std::endl;
			bits::print(_ifo.doubled_pawns[black]);

			std::cout << "\n  ===  white isolated === " << std::endl;
			bits::print(_ifo.isolated_pawns[white]);

			std::cout << "\n  ===  black isolated === " << std::endl;
			bits::print(_ifo.isolated_pawns[black]);

			std::cout << "\n  ===  white backward === " << std::endl;
			bits::print(_ifo.backward_pawns[white]);

			std::cout << "\n  ===  black backward === " << std::endl;
			bits::print(_ifo.backward_pawns[black]);

			std::cout << "\n  ===  light/dark sq pawns white === " << std::endl;
			bits::print(_ifo.pawnInfo.lightSqPawns[white]);
			bits::print(_ifo.pawnInfo.darkSqPawns[white]);

			std::cout << "\n  ===  light/dark sq pawns black === " << std::endl;
			bits::print(_ifo.pawnInfo.lightSqPawns[black]);
			bits::print(_ifo.pawnInfo.darkSqPawns[black]);

			std::cout << "\n  ===  white passed === " << std::endl;
			bits::print(_ifo.passed_pawns[white]);

			std::cout << "\n  ===  black passed === " << std::endl;
			bits::print(_ifo.passed_pawns[black]);

			std::cout << "\n  ===  white chain tips === " << std::endl;
			bits::print(_ifo.pawnInfo.chainTips[white]);

			std::cout << "\n  ===  black chain tips === " << std::endl;
			bits::print(_ifo.pawnInfo.chainTips[black]);

			std::cout << "\n  ===  white chain bases === " << std::endl;
			bits::print(_ifo.pawnInfo.chainBases[white]);

			std::cout << "\n  ===  black chain bases === " << std::endl;
			bits::print(_ifo.pawnInfo.chainBases[black]);

			std::cout << "\n  ===  white king shelter === " << std::endl;
			bits::print(_ifo.kingShelter[white]);

			std::cout << "\n  ===  black king shelter === " << std::endl;
			bits::print(_ifo.kingShelter[black]);
		}

		// TODO:
		//  - square scores
		//  - material
		//  - majorities
		//  - color complexes
		//  - passed pawns
		//  - undermining (levers)
		//  - king shelter
		//  - central control
		//  - pawn storms

		// Middlegame
		_ifo.mg.pawn_scores.push_back(eval_pawn_island_mg(white, p) - eval_pawn_island_mg(black, p));
		_ifo.mg.pawn_scores.push_back(eval_doubled_pawn_mg(white, p) - eval_doubled_pawn_mg(black, p));
		_ifo.mg.pawn_scores.push_back(eval_isolated_pawn_mg(white, p) - eval_isolated_pawn_mg(black, p));
		_ifo.mg.pawn_scores.push_back(eval_backward_pawn_mg(white, p) - eval_backward_pawn_mg(black, p));

		auto mgScore = std::accumulate(_ifo.mg.pawn_scores.begin(), _ifo.mg.pawn_scores.end(), 0);

		// Endgame
		_ifo.eg.pawn_scores.push_back(eval_pawn_island_eg(white, p) - eval_pawn_island_eg(black, p));
		_ifo.eg.pawn_scores.push_back(eval_doubled_pawn_eg(white, p) - eval_doubled_pawn_eg(black, p));
		_ifo.eg.pawn_scores.push_back(eval_isolated_pawn_eg(white, p) - eval_isolated_pawn_eg(black, p));
		_ifo.eg.pawn_scores.push_back(eval_backward_pawn_eg(white, p) - eval_backward_pawn_eg(black, p));

		auto egScore = std::accumulate(_ifo.eg.pawn_scores.begin(), _ifo.eg.pawn_scores.end(), 0);

		// ----- Post-process scores and return ---- //
		score = (int)std::round(
			mgScore * (1.0 - _ifo.egCoeff) + _ifo.egCoeff * egScore);

		return score;
	}

}

