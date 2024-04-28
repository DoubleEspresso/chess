
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
			std::vector<float> material_vals{ 0.0f, 300.0f, 315.0f, 480.0f, 910.0f };
		} mg;

		struct  {
			std::vector<float> material_vals{ 0.0f, 300.0f, 315.0f, 480.0f, 910.0f };
		} eg;
	};


	// Evaluation data
	// This is a collection of *static* features of the position to be scored
	struct EData {

		pawn_entry * pe = nullptr;
		material_entry * me = nullptr;

		// Placeholder to evaluate the opening game (tbd)
		struct {

		} og;

		struct {
			struct  {

			} pawnInfo;

			struct  {

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
	};


	class Evaluation {

	public:
		// Constructs an Edata object and calls internal evaluation methods.
		int evaluate(const position& p, const Searchthread& t, const float& lazy_margin);

		// Parameter optimization methods
		/*int update_params();*/

	private:
		/*Members*/
		ParamInfo _pInfo;
		EData _dInfo;

		/*State data*/
		bool _trace = false;
		bool _tune = false;

		/*Initialization*/
		void initalize(const position& p, const Searchthread& t);

		/*Material evaluation specific*/
		int eval_material(const position& p, const Searchthread& t);
		int eval_material(const Color& stm, const position& p);
		int eval_material_mg(const Color& stm, const position& p);
		int eval_material_eg(const Color& stm, const position& p);

		/*Pawn evaluation specific*/
		int eval_pawns(const position& p, const Searchthread& t);
		int eval_pawns(const Color& stm, const position& p);
		int eval_pawn_island_mg(const Color& stm, const position& p);
		int eval_pawn_island_eg(const Color& stm, const position& p);

		/*Knight evaluation specific*/


		/*Bishop evaluation specific*/
		/*Rook evaluation specific*/
		/*Queen evaluation specific*/
		/*King evaluation specific*/
	};
}

#endif
