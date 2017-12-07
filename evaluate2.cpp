#include "evaluate.h"
#include "definitions.h"
#include "squares.h"
#include "globals.h"
#include "utils.h"
#include "magic.h"
#include "material.h"
#include "pawns.h"
#include "opts.h"

using namespace Globals;
using namespace Magic;

namespace
{
	struct EvalInfo
	{
		int stm;
		GamePhase phase;
		U64 white_pieces;
		U64 black_pieces;
		U64 all_pieces;
		U64 empty;
		U64 white_pawns;
		U64 black_pawns;
		U64 w_pinned;
		U64 w_pinners;
		int b_king_attacks;
		int w_king_attacks;
		U64 b_pinned;
		U64 b_pinners;
		int white_ks;
		int black_ks;
		int tempoBonus;
		MaterialEntry * me;
		PawnEntry * pe;
		bool trace;
	};

	// bonuses/scores/definitions/positional weights
#define S(x,y) pick_score(x,y)
	inline int pick_score(int x, int y) { return (x + 1000 * y); }

	int PieceMobility[PIECES][32] =
	{ {}, // pawn
	  {S(-60,-70),S(-30,-40),S(0,-15),S(10,0),S(20,15),S(30,40),S(40,60),S(80,100)}, // knight

	  {S(-40,-50),S(-30,-50),S(-30,-50),S(-20,-45),S(-15,-30),S(-10,-35),S(0,-10),
	   S(10,5),S(20,20),S(30,30),S(40,50),S(50,60),S(60,70),S(80,100)}, // bishop

	  {S(-40,-50),S(-30,-50),S(-30,-50),S(-20,-45),S(-15,-30),S(-10,-35),S(0,-10),
	   S(10,5),S(20,20),S(30,30),S(40,50),S(50,60),S(60,70),S(80,100)}, // rook

	  // middle game mobility scores for queen are conservative
	  {S(-5,-100),S(-5,-100),S(-5,-100),S(-5,-100),S(0,-90),S(0,-80),S(0,-70),
	   S(0,-60),S(5,-50),S(5,-40),S(5,-30),S(10,-20),S(10,-10),S(10,0),
	   S(20,10),S(20,20),S(20,30),S(30,40),S(30,40),S(40,50),S(50,60),
	   S(60,70),S(70,80),S(80,90),S(90,100),S(100,110),S(110,120),S(120,130)}, // queen      
	  {} // king 
	};

	// piece attacks : [attacker][victim]
	int PieceAttacks[5][6] =
	{
		// pawn,    knight,   bishop,   rook,       queen,       king
		{ S(30,35), S(40,45), S(50,55), S(60,65), S(95, 100), S(100, 110) }, //pawn
		{ S(5,22), S(35,42), S(42,44), S(75,85), S(95, 100), S(100, 100) }, //knight
		{ S(5,22), S(35,42), S(42,44), S(75,85), S(95, 100), S(100, 100) }, //bishop
		{ S(5,22), S(35,42), S(32,34), S(45,50), S(95, 100), S(95, 100) }, //rook
		{ S(5,10), S(30,33), S(30,33), S(45,45), S(40, 55), S(95, 100) }, //queen
	};

	// [pinned piece][piece pinned to]
	int PinPenalty[PIECES - 1][PIECES] =
	{
	  { S(0,0), S(-2,-3), S(-3,-4), S(-6,-10), S(-15,-20), S(-18,-25) }, // pawn
	  { S(0,0), S(-1,-2) , S(-2,-3), S(-10,-12), S(-20,-30), S(-30,-40) }, // knight
	  { S(0,0), S(-1,-2) , S(-1,-2), S(-10,-12), S(-20,-30), S(-30,-40) }, // bishop
	  { S(0,0), S(-1,-2) , S(-1,-2), S(-1,-2), S(-20,-30), S(-70,-80) }, // rook
	  { S(0,0), S(-1,-2) , S(-1,-2), S(-1,-2), S(-1,-2), S(-90,-100) } // queen
	};

	// center influence [piece]			pawn,	knight,	bishop,	rook,	queen
	int CenterInfluence[PIECES - 1] = { S(20,1), S(4,0), S(3,0), S(3,0), S(2,0)  };

	// king pressure pawn, knight, bishop, rook, queen
	int KingPressure[PIECES - 1] = { S(20, 20), S(45,55), S(55,65), S(75,80), S(95,100) };
	U64 KingSafetyVision[2] = { 0ULL, 0ULL };

	// positional
	U64 SpaceMask[2] = { 0ULL, 0ULL };
	U64 CenterMask[2] = { 0ULL, 0ULL };

	int MaterialMG[5] = { PawnValueMG, KnightValueMG, BishopValueMG, RookValueMG, QueenValueMG };
	int MaterialEG[5] = { PawnValueEG, KnightValueEG, BishopValueEG, RookValueEG, QueenValueEG };
	int ClosedPositionBonus[2] = { 35, 45 };
	int OpenPositionBonus[2] = { 45, 50 };
	int BishopFriendlyPawnPenalty[2] = { -30, -45 };
	int BishopFriendlyPawnBonus[2] = { 20, 35 };
	int BishopEnemyPawnPenalty[2] = { -10, -35 };
	int BishopEnemyPawnBonus[2] = { 20, 35 };
	int DoubleBishopBonus[2] = { 60, 80 };
	int KingColorWeaknessBonus[2] = { 45, 65 };
	int KingExposurePenalty[2] = { 5, 2 };
	int KingUncastledPenalty[2] = { 25, 1 };
#undef S

	/*main evaluation methods*/
	int eval(Board& b);

	void traceout(EvalInfo& e); // debug output

	template<Color c> int eval_squares(Board& b, EvalInfo& ei);

	/*piece specific evaluations */
	template<Color c> int eval_pieces(Board&b, EvalInfo& ei);
	template<Color c, Piece p> int eval_piece(Board& b, EvalInfo& ei);
	template<Color c, Piece p> int eval_mobility(Board& b, EvalInfo& ei, int from, U64& quiets);
	template<Color c, Piece p> int eval_attacks(Board& b, EvalInfo& ei, int from, U64& attacks);
	template<Color c, Piece p> int eval_pins(Board& b, EvalInfo& ei, int from);
	template<Color c> int eval_trapped(Board& b, EvalInfo& ei);
	template<Color c, Piece p> int eval_piece_tempo(Board&b, EvalInfo& ei, int from);
	template<Color c> int eval_color_complexes(Board&b, EvalInfo&ei, int from, int pawnCount);
	template<Color c> int eval_threats(Board& b, EvalInfo& ei);

	/* positional/control */
	template<Color c> int eval_space(Board& b, EvalInfo& ei);
	template<Color c> int eval_center_pressure(Board& b, EvalInfo& ei); // pawns and minors only (?)
	template<Color c> int eval_open_files(Board& b, EvalInfo& ei);
	template<Color c> int eval_outposts(Board& b, EvalInfo& ei, int from);
	//template<Color c> int eval_development(Board& b, EvalInfo& ei);

	/*king safety specific*/
	template<Color c> int eval_king(Board& b, EvalInfo& ei);
	template<Color c, Piece p> int eval_king_pressure(Board& b, EvalInfo& ei);
	template<Color c> int eval_pawn_storms(Board& b, EvalInfo& ei);

	/*passed pawn candidates*/
	template<Color c> int eval_passers(Board& b, EvalInfo& ei);

	/*misc utility*/
	template<Piece p> int mob_score(int phase, int nb);
	template<Piece p1, Piece p2> int pin_score(int phase);
	template<Piece p1> int attack_score(int phase, int p2);
	template<Piece p> int king_pressure_score(int phase);
	int center_pressure_score(int p, int phase);
};

namespace Eval
{
	int evaluate(Board& b) { return eval(b); }
};

namespace
{
	int eval(Board &b)
	{
		EvalInfo ei;
		ei.trace = options["trace eval"];

		ei.stm = b.whos_move();
		ei.me = material.get(b);
		ei.phase = ei.me->game_phase;
		ei.tempoBonus = (ei.phase == MIDDLE_GAME ? 33 : 44);//10 : 20); //16 : 22);
		ei.white_pieces = b.colored_pieces(WHITE);
		ei.black_pieces = b.colored_pieces(BLACK);
		ei.empty = ~b.all_pieces();
		ei.all_pieces = b.all_pieces();
		ei.white_ks = b.king_square(WHITE);
		ei.black_ks = b.king_square(BLACK);
		ei.white_pawns = b.get_pieces(WHITE, PAWN);
		ei.black_pawns = b.get_pieces(BLACK, PAWN);
		ei.pe = pawnTable.get(b, ei.phase);
		ei.w_pinned = b.pinned(WHITE);
		ei.b_pinned = b.pinned(BLACK);
		ei.w_king_attacks = 0;
		ei.b_king_attacks = 0;

		KingSafetyVision[WHITE] = KingVisionBB[WHITE][QUEEN][ei.white_ks] | NeighborColsBB[COL(ei.white_ks)] | NeighborRowsBB[ROW(ei.white_ks)];
		KingSafetyVision[BLACK] = KingVisionBB[BLACK][QUEEN][ei.black_ks] | NeighborColsBB[COL(ei.black_ks)] | NeighborRowsBB[ROW(ei.black_ks)];
		SpaceMask[WHITE] = (RowBB[ROW3] | RowBB[ROW4] | RowBB[ROW5]) & (ColBB[COL3] | ColBB[COL4] | ColBB[COL5] | ColBB[COL6]);
		SpaceMask[BLACK] = (RowBB[ROW6] | RowBB[ROW5] | RowBB[ROW4]) & (ColBB[COL3] | ColBB[COL4] | ColBB[COL5] | ColBB[COL6]);
		CenterMask[WHITE] = (SquareBB[D3] | SquareBB[E3] | SquareBB[C4] | SquareBB[D4] | SquareBB[E4] | SquareBB[D5] | SquareBB[E5]);
		CenterMask[BLACK] = (SquareBB[E6] | SquareBB[D6] | SquareBB[C5] | SquareBB[D5] | SquareBB[E5] | SquareBB[D4] | SquareBB[E4]);

		int score = 0;
		score += (ei.stm == WHITE ? ei.tempoBonus : -ei.tempoBonus);

		score += (eval_squares<WHITE>(b, ei) - eval_squares<BLACK>(b, ei));

		score += (eval_pieces<WHITE>(b, ei) - eval_pieces<BLACK>(b, ei))*0.1;

		score += (eval_space<WHITE>(b, ei) - eval_space<BLACK>(b, ei))*0.1;

		score += (eval_center_pressure<WHITE>(b, ei) - eval_center_pressure<BLACK>(b, ei))*0.1;

		score += (eval_threats<WHITE>(b, ei) - eval_threats<BLACK>(b, ei)); 
		
		score += (eval_king<WHITE>(b, ei) - eval_king<BLACK>(b, ei));
		
		score += ei.pe->value;

		// nb. rescaling based on nb_pieces encourages trades at all times (each trade increases the scaling)
		//float f = 110.0 * nb_pieces *(0.1 + 0.2 + 0.3);
		//float scaling = f == 0 ? 1 : (280.0 / f); // usually around 0.6-0.8
		score *= 0.85;// 0.35;// 0.85;
		
		//if (abs(score) - abs(ei.me->value) >= abs(ei.me->value)) score *= 0.65;
		score += ei.me->value;
		//b.print();
		//printf("c=%s, score=%d, scaled=%d, scaling=%.6f, (%d)\n", (b.whos_move() == WHITE ? "white" : "black"), s1, score, scaling, ei.me->value);
		return (b.whos_move() == WHITE ? score : -score);
	}

	template<Color c>
	int eval_pieces(Board&b, EvalInfo& ei)
	{
		int score = 0;
		score += eval_piece<c, KNIGHT>(b, ei);
		score += eval_piece<c, BISHOP>(b, ei);
		score += eval_piece<c, ROOK>(b, ei);
		score += eval_piece<c, QUEEN>(b, ei);
		return score;
	}

	template<Color c, Piece p>
	int eval_piece(Board& b, EvalInfo& ei)
	{
		int score = 0;
		int *sqs = 0; sqs = b.sq_of<p>(c);
		if (!sqs) return score;
		int pawnCount = 0;
		U64 pawns = (ei.white_pawns | ei.black_pawns);
		if (pawns) pawnCount = count(pawns);
		int bishopCount = 0;
		int eks = (c == WHITE ? ei.black_ks : ei.white_ks);
		int them = (c == WHITE ? BLACK : WHITE);
		U64 kingRegion = KingSafetyBB[them][eks];
		U64 backRank = (c == WHITE ? RowBB[ROW1] : RowBB[ROW8]);

		for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
		{
			// collect moves
			U64 mvs = 0ULL; U64 quiet_mvs = 0ULL; U64 captr_mvs = 0ULL;
			if (p == KNIGHT) mvs = PseudoAttacksBB(KNIGHT, from);
			else if (p == BISHOP || p == ROOK) mvs = attacks<p>(ei.all_pieces, from);
			else mvs = (attacks<ROOK>(ei.all_pieces, from) | attacks<BISHOP>(ei.all_pieces, from)); // queen

			quiet_mvs = mvs & ei.empty;
			captr_mvs = mvs & (c == WHITE ? ei.black_pieces : ei.white_pieces);

			// 1. attacker score
			score += eval_attacks<c, p>(b, ei, from, captr_mvs);

			// 2. mobility (using built pin bitboards)
			score += eval_mobility<c, p>(b, ei, from, quiet_mvs)*0.90;

			// 3. pins (part of mobility)
			if (c == b.whos_move()) score += eval_pins<c, p>(b, ei, from); // note: these are *negative* scores

			// 4. tempo (attacked by lesser valued piece)
			score += eval_piece_tempo<c, p>(b, ei, from);

			// 5. positional eval for minor pieces
			if (p == KNIGHT)
			{
				score += eval_outposts<c>(b, ei, from) * 0.5;
				if (ei.pe->blockedCenter && pawnCount >= 12) score += ClosedPositionBonus[ei.phase];
				if (SquareBB[from] & backRank) score -= 20;
			}

			if (p == BISHOP)
			{
				score += eval_color_complexes<c>(b, ei, from, pawnCount)*0.5;
				if ( !ei.pe->blockedCenter && pawnCount < 11 ) score += OpenPositionBonus[ei.phase];
				if (SquareBB[from] & backRank) score -= 10;
				if (SquareBB[from] & LongDiagonalsBB) score += 15;
				++bishopCount;
			}

			// 6. king pressure - note : need something like this (does help in specific 
			// scenarios, but misses multiple simple tactics in most other positions with uncommented.
			U64 kingAttacks = (quiet_mvs | captr_mvs) & kingRegion;
			if (kingAttacks)
			{
				score += king_pressure_score<p>(ei.phase);
				(c == WHITE ? ++ei.w_king_attacks : ++ei.b_king_attacks); // attackers of enemy king
			}

			// 7. connectedness

		}
		if (bishopCount >= 2) score += DoubleBishopBonus[ei.phase];
		return score;
	}

	template<Color c, Piece p>
	int eval_mobility(Board& b, EvalInfo& ei, int from, U64& mvs)
	{
		int score = 0;

		// remove sqs attacked by pawns
		U64 bb = mvs & ei.pe->attacks[c == WHITE ? BLACK : WHITE];
		mvs ^= bb;

		// restrict in case of pins
		U64 pinned_to_king = 0ULL; pinned_to_king = SquareBB[from] & (c == WHITE ? ei.w_pinned : ei.b_pinned);
		if (pinned_to_king != 0ULL) return score; // inaccurate //score -= ei.tempoBonus;

		score += mob_score<p>(ei.phase, count(mvs));
		return score;
	}

	template<Color c, Piece p>
	int eval_piece_tempo(Board&b, EvalInfo& ei, int from)
	{
		int score = 0;
		int them = c == WHITE ? BLACK : WHITE;
		U64 pattacks = ei.pe->attacks[them];
		U64 minors = b.get_pieces(them, BISHOP) | b.get_pieces(them, KNIGHT);
		U64 rooks = b.get_pieces(them, ROOK);
		U64 queens = b.get_pieces(them, QUEEN);
		if (SquareBB[from] & pattacks) score -= 2 * ei.tempoBonus;
		if (p >= ROOK)
		{
			U64 attackers = b.attackers_of(from);
			if (p == ROOK && (attackers & minors)) score -= ei.tempoBonus;
			if (p == QUEEN && ((attackers & minors) || (attackers & rooks))) score -= ei.tempoBonus;
		}
		return score;
	}

	template<Color c>
	int eval_color_complexes(Board&b, EvalInfo&ei, int from, int pawnCount)
	{
		int score = 0;
		bool light_bishop = SquareBB[from] & ColoredSquaresBB[WHITE];
		bool dark_bishop = SquareBB[from] & ColoredSquaresBB[BLACK];

		int them = c == WHITE ? BLACK : WHITE;
		bool enemy_dark_bishop = (b.get_pieces(BLACK, BISHOP) & ColoredSquaresBB[BLACK]);
		bool enemy_white_bishop = (b.get_pieces(WHITE, BISHOP) & ColoredSquaresBB[WHITE]);
		int ks = (c == WHITE ? ei.black_ks : ei.white_ks);

		// pawn info 
		U64 enemy_wsq_pawns = (them == WHITE ? ei.white_pawns & ColoredSquaresBB[WHITE] : ei.black_pawns & ColoredSquaresBB[WHITE]);
		U64 enemy_bsq_pawns = (them == WHITE ? ei.white_pawns & ColoredSquaresBB[BLACK] : ei.black_pawns & ColoredSquaresBB[BLACK]);
		U64 our_wsq_pawns = (them == WHITE ? ei.black_pawns & ColoredSquaresBB[WHITE] : ei.white_pawns & ColoredSquaresBB[WHITE]);
		U64 our_bsq_pawns = (them == WHITE ? ei.black_pawns & ColoredSquaresBB[BLACK] : ei.white_pawns & ColoredSquaresBB[BLACK]);

		if (light_bishop && (count(enemy_wsq_pawns) <= 2)) score += BishopEnemyPawnPenalty[ei.phase];
		if (dark_bishop && (count(enemy_bsq_pawns) <= 2)) score += BishopEnemyPawnPenalty[ei.phase];

		// color penalties -- too many pawns on same color -- in theory this is not so bad in endgames
		if (light_bishop && (count(our_wsq_pawns) >= 3)) score += BishopFriendlyPawnPenalty[ei.phase];
		if (dark_bishop && (count(our_bsq_pawns) >= 3)) score += BishopFriendlyPawnPenalty[ei.phase];

		// color weakness around enemy king
		U64 enemy_king_pawns = KingSafetyBB[them][ks] & (light_bishop ? ColoredSquaresBB[WHITE] : ColoredSquaresBB[BLACK]) & (them == WHITE ? ei.white_pawns : ei.black_pawns);
		if (light_bishop && enemy_king_pawns == 0ULL && !enemy_white_bishop) score += KingColorWeaknessBonus[ei.phase];
		if (dark_bishop && enemy_king_pawns == 0ULL && !enemy_dark_bishop) score += KingColorWeaknessBonus[ei.phase];

		return score;
	}

	template<Color c> 
	int eval_outposts(Board& b, EvalInfo& ei, int from)
	{
		int score = 0;
		U64 pawns = (c == WHITE ? ei.white_pawns : ei.black_pawns);
		U64 pawnSupportBB = ei.pe->attacks[c];
		bool supported = (pawnSupportBB & SquareBB[from]);
		if (!supported) return score;

		int rowBonus = (c == WHITE ? ROW(from) : 9 - ROW(from));
		int colBonus = 4 - ( 4 - COL(from) < 0 ? COL(from) - 4 : 4 - COL(from) );
		U64 OutpostBB = (c == WHITE ? RowBB[ROW4] | RowBB[ROW5] | RowBB[ROW6] | RowBB[ROW7] : RowBB[ROW5] | RowBB[ROW4] | RowBB[ROW3] | RowBB[ROW2]);
		OutpostBB &= ColBB[COL3] | ColBB[COL4] | ColBB[COL5] | ColBB[COL6];

		OutpostBB &= SquareBB[from];
		if (!OutpostBB) return score;
		return rowBonus * 2 + colBonus;
	}

	template<Color c>
	int eval_space(Board& b, EvalInfo& ei)
	{
		U64 pawns = (c == WHITE ? ei.white_pawns : ei.black_pawns);
		U64 spaceBB = SpaceMask[c];
		spaceBB ^= ei.pe->attacks[c == WHITE ? BLACK : WHITE];

		U64 behindPawnsBB = 0ULL;
		for (int col = COL3; col <= COL5; ++col)
		{
			U64 pawnBB = ColBB[col] & pawns;
			int from = pop_lsb(pawnBB); // imperfect for doubled pawns on same file
			behindPawnsBB |= SpaceBehindBB[c][from];
		}
		spaceBB &= behindPawnsBB;
		U64 pawnsToRemove = spaceBB & pawns;
		spaceBB ^= pawnsToRemove;
		return 7 * count(spaceBB);
	}

	template<Color c>
	int eval_center_pressure(Board& b, EvalInfo& ei)
	{
		int score = 0;
		U64 pawns = (c == WHITE ? ei.white_pawns : ei.black_pawns);
		U64 CenterBB = (SquareBB[D4] | SquareBB[E4] | SquareBB[D5] | SquareBB[E5]);
		U64 AdvancedCenterBB = (c == WHITE ? SquareBB[C6] | SquareBB[D6] | SquareBB[E6] | SquareBB[F6] : SquareBB[C3] | SquareBB[D3] | SquareBB[E3] | SquareBB[F3]);
		if (ei.phase == MIDDLE_GAME)
		{
			U64 CenterPawnsBB = CenterBB & pawns;
			score += 10 * count(CenterPawnsBB);

			U64 AdvancedPawnsBB = AdvancedCenterBB & pawns;
			score += 20 * count(AdvancedPawnsBB);
		}
		U64 SmallCenterBB = (SquareBB[D4] | SquareBB[E4] | SquareBB[D5] | SquareBB[E5]);
		while (CenterBB)
		{
			int sq = pop_lsb(CenterBB);
			U64 attackers = b.attackers_of(sq) & b.colored_pieces(c);
			while (attackers)
			{
				int f = pop_lsb(attackers); 
				score += center_pressure_score(b.piece_on(f), ei.phase);
			}
		}
		return score;
	}

	template<Color c, Piece p>
	int eval_attacks(Board& b, EvalInfo& ei, int from, U64& attacks)
	{
		int score = 0;
		int them = (c == WHITE ? BLACK : WHITE);
		int ks = (c == WHITE ? ei.white_ks : ei.black_ks);

		while (attacks)
		{
			int to = pop_lsb(attacks);
			score += attack_score<p>(ei.phase, b.piece_on(to));
		}
		return score;
	}

	template<Color c>
	int eval_threats(Board& b, EvalInfo& ei)
	{
		int score = 0;
		int enemy_ks = (c == WHITE ? ei.black_ks : ei.white_ks);
		int them = (c == WHITE ? BLACK : WHITE);

		U64 pawns = (c == WHITE ? ei.white_pawns : ei.black_pawns);
		U64 knights = (c == WHITE ? b.get_pieces(WHITE, KNIGHT) : b.get_pieces(BLACK, KNIGHT));
		U64 bishops = (c == WHITE ? b.get_pieces(WHITE, BISHOP) : b.get_pieces(BLACK, BISHOP));
		U64 queens = (c == WHITE ? b.get_pieces(WHITE, QUEEN) : b.get_pieces(BLACK, QUEEN));
		U64 rooks = (c == WHITE ? b.get_pieces(WHITE, ROOK) : b.get_pieces(BLACK, ROOK));
		U64 our_pawn_attacks = ei.pe->attacks[c];
		U64 center_pawns = pawns & CenterMask[c];
		U64 KingRegion = KingSafetyVision[them];

		// targets
		U64 passed_pawns = ei.pe->passedPawns[them];
		U64 isolated_pawns = ei.pe->isolatedPawns[them];
		U64 backward_pawns = ei.pe->backwardPawns[them];
		U64 doubled_pawns = ei.pe->doubledPawns[them];
		U64 chain_bases = ei.pe->chainBase[them];
		U64 undefended_pawns = ei.pe->undefended[them];
		U64 chain_heads = ei.pe->pawnChainTips[them];

		// weak pawn targets for pieces/pawns
		U64 piece_targetBB = (undefended_pawns & (doubled_pawns | backward_pawns | isolated_pawns | passed_pawns));
		U64 pawn_targetBB = (chain_bases | chain_heads | passed_pawns);

		if (piece_targetBB)
			while (piece_targetBB)
			{
				int sq = pop_lsb(piece_targetBB);
				int target_value = square_score<c, PAWN>(ei.phase, sq); // ranges from 5-100
				U64 attackers = b.attackers_of(sq);
				if ((attackers & knights) != 0ULL) score += target_value;
				if ((attackers & bishops) != 0ULL) score += target_value;
				if ((attackers & queens) != 0ULL) score += target_value;
				if ((attackers & rooks) != 0ULL) score += target_value;
			}

		if (pawn_targetBB)
			while (pawn_targetBB)
			{
				int sq = pop_lsb(pawn_targetBB);
				int target_value = square_score<c, PAWN>(ei.phase, sq); // ranges from 5-100
				U64 attackers = b.attackers_of(sq) & pawns;
				if (attackers != 0ULL) score += target_value;
			}

		// nb: this is a little dangerous since no tactics are resolved, it is a bonus for 
		// attacking a piece with a pawn on the move.
		U64 their_pawns = (them == WHITE ? ei.white_pawns : ei.black_pawns);
		U64 attacked_by_pawns = ((c == WHITE ? ei.black_pieces : ei.white_pieces) & our_pawn_attacks);		
		U64 their_pawns_attacked = attacked_by_pawns & their_pawns;
		attacked_by_pawns ^= their_pawns_attacked; // only pieces
		// pieces attacked by our pawns
		while (attacked_by_pawns) score += attack_score<PAWN>(ei.phase, pop_lsb(attacked_by_pawns));

		// bonus for discovered checkers 
		U64 disc_checkers = b.discovered_checkers(c);
		U64 disc_blockers = b.discovered_blockers(c);
		U64 enemy_pieces = (b.colored_pieces(them));
		if (disc_checkers)
		{
			while (disc_checkers)
			{
				int f = pop_lsb(disc_checkers);
				U64 disc_mask = (BetweenBB[f][enemy_ks]) & enemy_pieces;
				if (count(disc_mask) <= 1) score += 50;//80;
			}
			while (disc_blockers)
			{
				int f = pop_lsb(disc_blockers);
				if (b.piece_on(f) >= KNIGHT) score += 50;//150;// 125;
			}
		}

		// eval knight forks
		//U64 knight_forks = b.checkers() & knights;
		if (knights)
		{
			U64 target = (c == WHITE ? b.get_pieces(BLACK, ROOK) | b.get_pieces(BLACK, QUEEN) | b.get_pieces(BLACK, KING) :
				b.get_pieces(WHITE, ROOK) | b.get_pieces(WHITE, QUEEN) | b.get_pieces(WHITE, KING));
				while (knights)
				{
					int sq = pop_lsb(knights);
					U64 attacks = PseudoAttacksBB(KNIGHT, sq) & target;
					if (more_than_one(attacks)) score += 150;

				}

		}
		return score;
	}

	template<Color c>
	int eval_king(Board& b, EvalInfo& ei)
	{
		int score = 0;
		int them = (c == WHITE ? BLACK : WHITE);
		int eks = (c == BLACK ? ei.white_ks : ei.black_ks);

		U64 defenders = KingSafetyBB[them][eks] & (b.colored_pieces(them));
		int nb_defenders = count(defenders);
		int nb_attacks = (c == WHITE ? ei.w_king_attacks : ei.b_king_attacks); 
		//if (nb_defenders >= 3) return score;

		U64 our_pawns = (c == WHITE ? ei.white_pawns : ei.black_pawns);
		U64 their_pawns = (c == WHITE ? ei.black_pawns : ei.white_pawns);
		U64 our_king = SquareBB[(c == BLACK ? ei.black_ks : ei.white_ks)];

		U64 pawnsRank2 = (c == WHITE ? RowBB[ROW2] & our_pawns : RowBB[ROW7] & our_pawns);
		if (count(pawnsRank2) >= 6) return score;

		bool castled = b.is_castled((Color)them);

		U64 pieces = b.colored_pieces(c);
		U64 bishops = b.get_pieces(c, BISHOP);
		U64 rooks = b.get_pieces(c, ROOK);
		U64 queens = b.get_pieces(c, QUEEN);
		U64 knights = b.get_pieces(c, KNIGHT);
		U64 checkers = b.checkers();

		// eval king safety
		U64 mobility = PseudoAttacksBB(KING, eks);
		U64 local_attackers = KingSafetyBB[c][eks] & b.colored_pieces(c);

		int nb_safe = 0; int nb_attackers = 0;
		while (mobility)
		{
			int to = pop_lsb(mobility);
			U64 attackers = b.attackers_of(to) & pieces;
			bool empty = b.piece_on(to) == PIECE_NONE;
			if (attackers)// && empty)
			{
				if (attackers & our_pawns) score += 35;//35;
				if (attackers & queens) score += 25;//20;// 25;
				if (attackers & rooks) score += 20;// 20;
				if (attackers & knights) score += 15;// 15;
				if (attackers & bishops) score += 15;// 15;
				if (attackers & our_king) score += 40;//40;// 120; // nb. in pawn endgames this is not really dangerous
				++nb_attackers;
				// could be a mate threat
				if (more_than_one(attackers)) score += 25;//30;// 210;
			}
			else if (empty)
			{
				++nb_safe;
				score -= 20;// 25;
			}
		}

		//if (checkers != 0ULL)
		//{
		//	int nb_checking = 0;
		//	while (checkers)
		//	{
		//		score += ei.tempoBonus / 2;
		//		++nb_checking;
		//		int f = pop_lsb(checkers); int p = b.piece_on(f);
		//		bool contact_check = (PseudoAttacksBB(KING, eks) & SquareBB[f]);
		//		bool defended_check = (b.attackers_of(f) & b.colored_pieces(them));
		//		U64 other_attacks = PseudoAttacksBB(p, f) & b.colored_pieces(them);
		//		if ((p == PAWN || p >= ROOK) && contact_check)
		//		{
		//			score += 170;
		//			if (defended_check) score -= 25;
		//			if (nb_safe == 0) score += 120;
		//		}
		//		if (p == KNIGHT || p == BISHOP)
		//		{
		//			score += 5;
		//			if (nb_safe == 0) score += 70;
		//		}
		//		if (other_attacks) score += 70;
		//	}
		//	if (nb_checking > 1) score += 80;
		//}

		// approximate mate detection
		if (nb_safe <= 2 && defenders == 0ULL && nb_attackers >= 2)
		{
			score += 125;// 120;
			if (local_attackers) score += 40;// 25;
		}
		if (checkers != 0ULL && nb_safe <= 1)
		{
			score += 180;// 180;
			if (more_than_one(checkers)) score += 280;// 180;
			if (nb_safe == 0 && defenders == 0ULL && local_attackers) score += 180;
		}

		// bonus for enemy with no pawn cover in middle game
		if (ei.phase == MIDDLE_GAME)
		{
			U64 pawn_cover = KingSafetyBB[them][eks] & their_pawns;
			//if (pawn_cover) score -= count(pawn_cover);
			if (count(pawn_cover) < 2) score += 75;// 150;
		}

		// bonus for uncastled enemy king
		if (!castled) score += KingUncastledPenalty[ei.phase];
		if (!castled && !b.can_castle(c == WHITE ? ALL_W : ALL_B)) score += KingUncastledPenalty[ei.phase];

		return score;
	}

	template<Color c, Piece p>
	int eval_pins(Board& b, EvalInfo& ei, int from)
	{
		// nb: only checking pins against king and queen for now
		int score = 0;
		int them = (c == WHITE ? BLACK : WHITE);
		int ks = (c == WHITE ? ei.white_ks : ei.black_ks);
		U64 sliders = b.get_pieces(them, BISHOP) | b.get_pieces(them, ROOK) | b.get_pieces(them, QUEEN);
		U64 attackers = b.attackers_of(from) & sliders;

		// ensure value of pinning piece > pinned piece 
		// means we should rescale the penalty .. especially if we can capture the pinning piece etc.
		U64 pinned_to_king = 0ULL; pinned_to_king = SquareBB[from] & (c == WHITE ? ei.w_pinned : ei.b_pinned);
		if (pinned_to_king)
		{
			int s = pop_lsb(attackers); // imperfect for multiple pinning pieces!
			int mdiff = MaterialMG[p] - MaterialMG[b.piece_on(s)];
			if (mdiff > 0)
			{
				score += pin_score<p, KING>(ei.phase);
			}
			else score -= 0.5*ei.tempoBonus;
		}
		return score;
	}

	template<Color c>
	int eval_squares(Board& b, EvalInfo& e)
	{
		int score = 0;
		// pawn sqs
		int * sqs = b.sq_of<PAWN>(c);
		for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
			score += square_score<c, PAWN>(e.phase, from);

		// knight sqs
		sqs = b.sq_of<KNIGHT>(c);
		for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
			score += square_score<c, KNIGHT>(e.phase, from);

		// bishop sqs
		sqs = b.sq_of<BISHOP>(c);
		for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
			score += square_score<c, BISHOP>(e.phase, from);

		// rook sqs
		sqs = b.sq_of<ROOK>(c);
		for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
			score += square_score<c, ROOK>(e.phase, from);

		// queen sqs
		sqs = b.sq_of<QUEEN>(c);
		for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
			score += square_score<c, QUEEN>(e.phase, from);

		// king sq
		score += square_score<c, KING>(e.phase, (c == WHITE ? e.white_ks : e.black_ks));

		//scores[c].push_back(score);
		return score;
	}

	// misc
	template<Piece p> int mob_score(int phase, int nb)
	{
		int ss = PieceMobility[p][nb];
		int eg = ss / 1000;
		int mg = ss - 1000 * rounded((float)ss / (float)1000);
		return (phase == MIDDLE_GAME ? mg : eg);
	}
	// piece p1 is pinned to p2
	template<Piece p1, Piece p2> int pin_score(int phase)
	{
		int ss = PinPenalty[p1][p2];
		int eg = ss / 1000;
		int mg = ss - 1000 * rounded((float)ss / (float)1000);
		return (phase == MIDDLE_GAME ? mg : eg);
	}
	template<Piece p1> int attack_score(int phase, int p2)
	{
		int ss = PieceAttacks[p1][p2];
		int eg = ss / 1000;
		int mg = ss - 1000 * rounded((float)ss / (float)1000);
		return (phase == MIDDLE_GAME ? mg : eg);
	}
	template<Piece p> int king_pressure_score(int phase)
	{
		int ss = KingPressure[p];
		int eg = ss / 1000;
		int mg = ss - 1000 * rounded((float)ss / (float)1000);
		return (phase == MIDDLE_GAME ? mg : eg);
	}
	int center_pressure_score(int p, int phase)
	{
		int ss = CenterInfluence[p];
		int eg = ss / 1000;
		int mg = ss - 1000 * rounded((float)ss / (float)1000);
		return (phase == MIDDLE_GAME ? mg : eg);
	}

};
