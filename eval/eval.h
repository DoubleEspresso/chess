
#pragma once

#ifndef _EVALUATE_H
#define _EVALUATE_H

#include "../types.h"
#include "../utils.h"
#include "../pawns.h"
#include "../material.h"
#include "../parameter.h"
#include "../position.h"
#include "../utils.h"
#include "../threads.h"

namespace Evaluation {

	/// <summary>
	/// Evaluation parameter info for middle game and endgame
	/// Extend to a separate class with read/write/udpate methods
	/// </summary>
	struct ParamInfo {

		struct  {

		} og;

		struct  {
			int tempo = 15;
			std::vector<int> material_vals{ 100, 300, 315, 480, 910 };

			/*Pawn eval specific*/
			std::vector<int> islandPenalty { 0, 2, 6, 12, 36, 72 };
			std::vector<int> doubledPenalty{ 0, 6, 6, 12, 12, 24, 24, 72 };
			std::vector<int> doubledOpen{ 0, 12, 12, 24, 24, 72, 72, 144 };
			int doubledDefendedBonus = 2;
			int isolatedPenalty = 4; // Only applied to pawns on flanks
			int isolated2BishopsBonus = 6; // Bonus for having 2 bishops with an isolani
			int isolatedKnightPostBonus = 1; // Bonus for having a knight posted at the isolani attack pts (valid?)
			int isolatedPassedPenalty = 2; // Rational is that this pawn is now easier to attack
			int isolatedControlPenalty = 6; // Blockaded passed pawn is a problem
			std::vector<int> isoFileBonus {0, 0, 0, 4, 2, 0}; // pawn, knight, bishop, rook, queen, king
			int backwardPenalty = 8; // Backward pawns are generally weak (no benefits)
			int backwardNeighborPenalty = 4; // Backward pawns are harder to advance with enemy neighbors
			int backwardControlPenalty = 10; // Backward pawns are generally weak (no benefits)

		} mg;

		struct  {
			int tempo = 15;
			std::vector<int> material_vals{ 115, 285, 330, 495, 895 };

			/*Pawn eval specific*/
			std::vector<int> islandPenalty{ 0, 1, 3, 6, 18, 36 };
			std::vector<int> doubledPenalty{ 0, 12, 12, 24, 24, 72, 72, 144 };
			std::vector<int> doubledOpen{ 0, 24, 242, 48, 48, 96, 96, 144 };
			int doubledDefendedBonus = 4;
			int isolatedPenalty = 14; // All isolated pawns are considered much weaker now
			int isolatedControlPenalty = 12; // Blockaded passed pawn is a problem
			int backwardPenalty = 14; // Backward pawns are generally weak (no benefits)
			int backwardNeighborPenalty = 8; // Backward pawns are harder to advance with enemy neighbors
			int backwardControlPenalty = 15; // Backward pawns are generally weak (no benefits)

		} eg;
	};


	// Evaluation data
	// This is a collection of *static* features of the position to be scored
	struct EData {

		pawn_entry * pe = nullptr;
		material_entry * me = nullptr;

		// The interpolation coefficient between endgame and middlegame - computed from minor piece material
		float egCoeff = 0.0f;

		// Placeholder to evaluate the opening game (tbd)
		struct {

		} og;

		struct {
			struct  {
			} pawnInfo;

			struct  {
				int sqScore = 0;

				int eval = 0;
			} knightInfo;

			struct  {

			} bishopInfo;

			struct  {

			} queenInfo;

			struct  {

			} kingInfo;

			// etc..

			// The symmetrized scores for each evaluation 'chunk' for the middlegame
			std::vector<int> mat_scores;
			std::vector<int> pawn_scores;
			std::vector<int> scores;
		} mg;

		struct  {

			// The symmetrized scores for each evaluation 'chunk' for the endgame
			std::vector<int> mat_scores;
			std::vector<int> pawn_scores;
			std::vector<int> scores;
		} eg;

		/*Useful position data*/
		struct {
			std::vector<U64> wPawnIslands;
			std::vector<U64> bPawnIslands;
			U64 qPawnMajorities[2] = { 0ULL, 0ULL };
			U64 kPawnMajorities[2] = { 0ULL, 0ULL };
			U64 lightSqPawns[2] = { 0ULL, 0ULL };
			U64 darkSqPawns[2] = { 0ULL, 0ULL };
			U64 defended[2] = { 0ULL, 0ULL };
			U64 undefended[2] = { 0ULL, 0ULL };
			U64 chainBases[2] = { 0ULL, 0ULL };
			U64 chainTips[2] = { 0ULL, 0ULL };
		} pawnInfo;

		U64 kingShelter[2] = { 0ULL, 0ULL };
		U64 pieces[2] = { 0ULL, 0ULL };
		U64 empty = 0ULL;
		U64 all_pieces = 0ULL;
		U64 open_files = 0ULL;
		U64 semiOpen_files[2] = { 0ULL, 0ULL };
		U64 doubled_pawns[2] = { 0ULL, 0ULL };
		U64 isolated_pawns[2] = { 0ULL, 0ULL };
		U64 backward_pawns[2] = { 0ULL, 0ULL };
		U64 passed_pawns[2] = { 0ULL, 0ULL };
		U64 pawn_attacks[2] = { 0ULL, 0ULL };
	};


	class Evaluation {

	public:
		// Constructs an Edata object and calls internal evaluation methods.
		int evaluate(const position& p, const Searchthread& t, const float& lazy_margin);

		// Parameter optimization methods
		void Initialize(const std::vector<int>& parameters);

	private:
		/*Members*/
		ParamInfo _pIfo;
		EData _ifo;

		/*State data*/
		bool _trace = true;
		bool _tune = false;

		/*Initialization*/
		void initalize(const position& p, const Searchthread& t);

		/*Material evaluation specific*/
		int eval_material(const position& p, const Searchthread& t);
		int eval_material(const position& p, material_entry& e);
		int eval_material_mg(const Color& stm, const position& p, material_entry& e);
		int eval_material_eg(const Color& stm, const position& p, material_entry& e);


		/*Pawn evaluation specific*/
		int eval_pawns(const position& p, const Searchthread& t);
		int eval_pawns(const position& p, pawn_entry& e);
		int eval_pawn_island_mg(const Color& stm, const position& p);
		int eval_pawn_island_eg(const Color& stm, const position& p);
		int eval_doubled_pawn_mg(const Color& stm, const position& p);
		int eval_doubled_pawn_eg(const Color& stm, const position& p);
		int eval_isolated_pawn_mg(const Color& c, const position& p);
		int eval_isolated_pawn_eg(const Color& c, const position& p);
		int eval_backward_pawn_mg(const Color& c, const position& p);
		int eval_backward_pawn_eg(const Color& c, const position& p);

		// TODO...
		/*
		int eval_doubled_pawns_mg(const Color& stm, const position& p, pawn_entry& e);
		int eval_doubled_pawns_eg(const Color& stm, const position& p, pawn_entry& e);
		int eval_pawn_holes_mg(const Color& stm, const position& p, pawn_entry& e);
		int eval_pawn_holes_eg(const Color& stm, const position& p, pawn_entry& e);
		int eval_isolated_mg(const Color& stm, const position& p, pawn_entry& e);
		int eval_isolated_eg(const Color& stm, const position& p, pawn_entry& e);
		int eval_majorities_mg(const Color& stm, const position& p, pawn_entry& e);
		int eval_majorities_eg(const Color& stm, const position& p, pawn_entry& e);
		int eval_passed_mg(const Color& stm, const position& p, pawn_entry& e);
		int eval_passed_eg(const Color& stm, const position& p, pawn_entry& e);
		int eval_shelter_mg(const Color& stm, const position& p, pawn_entry& e);
		int eval_shelter_eg(const Color& stm, const position& p, pawn_entry& e);
		int eval_undermine_mg(const Color& stm, const position& p, pawn_entry& e);
		int eval_undermine_eg(const Color& stm, const position& p, pawn_entry& e);
		int eval_pawn_control_mg(const Color& stm, const position& p, pawn_entry& e);
		int eval_pawn_control_eg(const Color& stm, const position& p, pawn_entry& e);
		int eval_pawn_color_mg(const Color& stm, const position& p, pawn_entry& e);
		int eval_pawn_color_eg(const Color& stm, const position& p, pawn_entry& e);
		int eval_pawn_center_mg(const Color& stm, const position& p, pawn_entry& e);
		int eval_pawn_center_eg(const Color& stm, const position& p, pawn_entry& e);
		int eval_backward_mg(const Color& stm, const position& p, pawn_entry& e);
		int eval_backward_eg(const Color& stm, const position& p, pawn_entry& e);
		int eval_pawn_levers_mg(const Color& stm, const position& p, pawn_entry& e);
		int eval_pawn_levers_eg(const Color& stm, const position& p, pawn_entry& e);
		int eval_pawn_storm_mg(const Color& stm, const position& p, pawn_entry& e);
		int eval_pawn_storm_eg(const Color& stm, const position& p, pawn_entry& e);
		*/


		/*Knight evaluation specific*/
		int eval_knight_mg(const Color& c, const position& p);
		int eval_knight_eg(const Color& c, const position& p);

		/*Bishop evaluation specific*/
		/*Rook evaluation specific*/
		/*Queen evaluation specific*/
		/*King evaluation specific*/
	};
}

#endif
