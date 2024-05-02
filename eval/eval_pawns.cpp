
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
			for (int c = 0; c < 8; ++c) {
				auto filePawns = bitboards::col[c] & pawns;
				if (filePawns && bits::more_than_one(filePawns))
					doubled |= filePawns;
			}
			e.doubled_pawns[c] = doubled;
		}

		inline void find_isolated_pawns(const Color& c, const position& p, EData& e) {
			auto pawns = p.get_pieces<pawn>(c);
			U64 isolated = 0ULL;
			for (int c = 0; c < 8; ++c) {
				auto neighbors = bitboards::neighbor_cols[c] & pawns;
				if (!neighbors)
					isolated |= bitboards::col[c] & pawns;
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

		// pawns undefended by pawns - members of islands are 'bases' of chains

		// chain 'tips' are the 'most' advanced members of islands

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
		}

		// Middlegame
		_ifo.mg.pawn_scores.push_back(eval_pawn_island_mg(white, p, e) - eval_pawn_island_mg(black, p, e));

		// Endgame
		_ifo.eg.pawn_scores.push_back(eval_pawn_island_eg(white, p, e) - eval_pawn_island_eg(black, p, e));

		// Post process scores
		return score;
	}

}

