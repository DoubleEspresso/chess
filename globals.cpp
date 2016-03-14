#include "globals.h"

namespace Globals
{

	U64 RowBB[ROWS];
	U64 ColBB[COLS];
	U64 ColorBB[COLORS];               // colored squares (white or black)
	U64 NeighborColsBB[COLS];          // returns the neighbor columns (for isolated pawns)
	U64 PassedPawnBB[COLORS][SQUARES]; // in front of and each side of the pawn
	U64 BetweenBB[SQUARES][SQUARES];   // bitmap of squares aligned between 2 squares, used for pins
	U64 SquareBB[SQUARES+2];             // store each square as a U64 for fast access
	U64 SmallCenterBB;
	U64 BigCenterBB;
	U64 CornersBB;

	// evaluation/utility bitmaps
	U64 PawnAttacksBB[COLORS][SQUARES];          // the pawn attacks (by color)
	U64 AttacksBB[PIECES - 1][SQUARES];            // knight,bishop,rook,queen,king
	U64 BishopMask[SQUARES];                     // bishop mask (outer board edges are trimmed)
	U64 RookMask[SQUARES];                       // rook mask (outer board edges are trimmed)
	U64 QueenMask[SQUARES];                      // queen mask (outer board edges are trimmed)
	U64 CastleMask[2][2];                        // determine castle state of each side
	U64 KingSafetyBB[COLORS][SQUARES];           // generic array around king (for pawn shelter computation)
	U64 KingVisionBB[COLORS][PIECES - 1][SQUARES]; // pseudo attack bitboards centered at each king-square
	U64 SpaceBehindBB[COLORS][SQUARES];          // all space behind a square
	U64 SpaceInFrontBB[COLORS][SQUARES];         // all space in front of a square
	U64 ColsInFrontBB[COLORS][SQUARES];          // all square between current square and queening square
	U64 ColoredSquaresBB[COLORS];					 // squares of a specific color
	U64 CenterMaskBB;


	// the main initialization routine, which loads all the 
	// bitboard data listed in the globals namespace, these 
	// data are used to compute move hashes for the bishop/rook 
	// as well as aid the evaluation/searching methods
	bool init()
	{
		// fill the square bitboard - a lookup table for
		// converting each square into a U64 data type
		for (int s = 0; s < SQUARES+2; ++s) SquareBB[s] = U64(s);

		// init the colored squares bitboard - a lookup table
		// useful for the bishop evaluations/color weaknesses
		ColoredSquaresBB[WHITE] = 0ULL;
		ColoredSquaresBB[BLACK] = 0ULL;

		// the center mask (smaller center - just 4 squares)
		CenterMaskBB = (SquareBB[D4] | SquareBB[D5] | SquareBB[E4] | SquareBB[E5]);

		// fill the rows/cols bitboards
		for (int r = 0; r < 8; ++r)
		{
			U64 row = 0ULL; U64 col = 0ULL;

			for (int c = 0; c < 8; ++c)
			{
				col |= SquareBB[c * 8 + r];
				row |= SquareBB[r * 8 + c];
			}
			RowBB[r] = row;
			ColBB[r] = col;
		}

		// helpful definitions for board corners/edges
		U64 board_edge = RowBB[ROW1] | ColBB[COL1] | RowBB[ROW8] | ColBB[COL8];
		CornersBB = SquareBB[A1] | SquareBB[H1] | SquareBB[H8] | SquareBB[A8];

		// attack step data
		int pawn_deltas[2][2] = { { 9,7 },{ -7,-9 } }; // one entry for each color
		int knight_deltas[8] = { -17, -15, -10, -6, 6, 10, 15, 17 };
		int king_deltas[8] = { 8,  -8,  -1,  1, 9,  7, -9, -7 };
		int bishop_deltas[4] = { 9,  -7,   7, -9 };
		int rook_deltas[4] = { 8,  -8,   1, -1 };
		int k_safety[2][3] = { { 16, 15, 17 },{ -16,-15,-17 } };
		int p_safety[6] = { 7,  8, 9,  16, 15, 17 };

		// main loop over all board squares
		for (int s = A1; s <= H8; ++s)
		{
			// the colored squares bitboard, black squares
			// exist on even rows, and even squares, all other squares are white
			if (ROW(s) % 2 == s % 2 == 0) ColoredSquaresBB[BLACK] |= SquareBB[s];
			else ColoredSquaresBB[WHITE] |= SquareBB[s];

			// load the pawn attack bitmaps here
			for (int c = WHITE; c <= BLACK; c++)
			{
				PawnAttacksBB[c][s] = 0ULL;
				for (int step = 0; step < 2; step++)
				{
					int to = s + pawn_deltas[c][step];

					if (on_board(to) && row_distance(s, to) < 2 && col_distance(s, to) < 2)
					{
						PawnAttacksBB[c][s] |= SquareBB[to];
					}
				}
			}


			// load the knight and king attack bitmaps here
			U64 knight_attacks = 0ULL;
			U64 king_attacks = 0ULL;
			for (unsigned step = 0; step < 8; ++step)
			{
				// the knight
				int to = s + knight_deltas[step];
				if (on_board(to) && row_distance(s, to) <= 2 && col_distance(s, to) <= 2)
				{
					knight_attacks |= SquareBB[to];
				}

				// the king
				to = s + king_deltas[step];
				if (on_board(to) && row_distance(s, to) <= 1 && col_distance(s, to) <= 1)
				{
					king_attacks |= SquareBB[to];
				}
			} // end loop over knight/king steps

			if (knight_attacks) AttacksBB[0][s] = knight_attacks;
			if (king_attacks) AttacksBB[4][s] = king_attacks;


			// load the bishop attacks here
			U64 bishop_attacks = 0ULL;

			for (unsigned step = 0; step < 4; ++step)
			{
				int j = 1;
				while (true)
				{
					int to = s + (j++) * bishop_deltas[step];

					if (on_board(to) && on_diagonal(s, to))
					{
						bishop_attacks |= SquareBB[to];
					}
					else break;
				}
			}
			if (bishop_attacks)
			{
				AttacksBB[1][s] = bishop_attacks;

				// the trimmed bishop mask
				U64 tmp = bishop_attacks & board_edge;
				BishopMask[s] = bishop_attacks ^ tmp;
			}


			// load the rook attacks here
			U64 rook_attacks = 0ULL;
			for (unsigned step = 0; step < 4; ++step)
			{
				int j = 1;
				while (true)
				{
					int to = s + (j++) * rook_deltas[step];
					if (on_board(to) && (same_row(s, to) || same_col(s, to)))
					{
						rook_attacks |= SquareBB[to];
					}
					else break;
				}
			}

			// remove the outer bits for the rook masks
			if (rook_attacks)
			{
				U64 col_maxmin = (SquareBB[COL(s)]) | (SquareBB[(56 + COL(s))]);
				U64 row_maxmin = (SquareBB[8 * ROW(s)]) | (SquareBB[(8 * ROW(s) + 7)]);

				U64 m = col_maxmin | row_maxmin;

				RookMask[s] = rook_attacks ^ m;
				if (RookMask[s] & SquareBB[s]) RookMask[s] = RookMask[s] ^ SquareBB[s];

				AttacksBB[2][s] = rook_attacks;
			}

			// the queen attacks
			AttacksBB[3][s] = AttacksBB[1][s] | AttacksBB[2][s];

			// the queen masks
			QueenMask[s] = RookMask[s] | BishopMask[s];


			// Positional arrays : fill the evaluation/safety/utility arrays

			// 1. the passed pawn masks and isolated pawn masks in one shot
			// region of interest for passed pawns excludes the back ranks for either color
			U64 roi = ~(RowBB[COL1] | RowBB[COL8]);

			if (SquareBB[s] & roi)
			{
				// overwrite neighbor cols multiple times here, but should be fine
				NeighborColsBB[COL(s)] = 0ULL;

				for (int color = WHITE; color <= BLACK; ++color)
				{
					PassedPawnBB[color][s] = 0ULL;
					U64 neighbor_squares = (AttacksBB[4][s] & RowBB[ROW(s)]) | SquareBB[s];

					while (neighbor_squares)
					{
						int square = pop_lsb(neighbor_squares);
						PassedPawnBB[color][s] |= infrontof_square(ColBB[COL(square)], color, square);

						// collect just the neighbor columns for a given square
						if (s != square) NeighborColsBB[COL(s)] |= ColBB[COL(square)];
					}

				}
			}

			// 2. king safety bitboards, indexed by color
			// they are generic regions around king, to count pawn cover
			for (int color = WHITE; color <= BLACK; ++color)
			{
				KingSafetyBB[color][s] = AttacksBB[4][s];

				for (unsigned step = 0; step < 3; ++step)
				{
					int to = s + k_safety[color][step];

					if (on_board(to) && row_distance(s, to) <= 2 && col_distance(s, to) <= 2) KingSafetyBB[color][s] |= SquareBB[to];
				}
			}

			// 3. king vision bitboards, indexed by square
			// these arrays track all possible attacks to the king sitting at a given square
			for (int color = WHITE; color <= BLACK; ++color)
			{
				KingVisionBB[color][PAWN][s] = PawnAttacksBB[color][s]; KingVisionBB[color][KNIGHT][s] = AttacksBB[0][s];
				KingVisionBB[color][BISHOP][s] = AttacksBB[1][s]; KingVisionBB[color][ROOK][s] = AttacksBB[2][s];
				KingVisionBB[color][QUEEN][s] = AttacksBB[3][s];
			}

			// 4. the between bitboard, used for determining pins
			for (int s2 = A1; s2 <= H8; ++s2)
			{
				if (aligned(s, s2) && s != s2)
				{
					U64 btwn = 0ULL;

					int iter = 0;
					int delta = 0;

					int sq;

					if (col_distance(s, s2) == 0) delta = (s < s2 ? 8 : -8);
					else if (row_distance(s, s2) == 0) delta = (s < s2 ? 1 : -1);
					else if (on_diagonal(s, s2))
					{
						if (s < s2 && COL(s) < COL(s2)) delta = 9;
						else if (s < s2 && COL(s) > COL(s2)) delta = 7;
						else if (s > s2 && COL(s) < COL(s2)) delta = -7;
						else delta = -9;
					}

					if (delta != 0)
					{
						do
						{
							sq = s + iter * delta;
							btwn |= SquareBB[sq];
							iter++;
						} while (sq != s2);
					}
					BetweenBB[s][s2] = btwn;
				}
			}

			
			// 5. the space behind and in front of a given square (exluding the row of the square)
			for (int c = WHITE; c <= BLACK; ++c)
			{
				SpaceInFrontBB[c][s] = (c == WHITE ? BetweenBB[s][COL(s) + 7] : BetweenBB[s][COL(s)]);
				SpaceBehindBB[c][s] = (c == WHITE ? BetweenBB[s][COL(s)] : BetweenBB[s][COL(s) + 7]);
			}
			

		} // end loop over board squares

		// 6. the castle masks
		CastleMask[WHITE][0] = (U64(B1) | U64(C1) | U64(D1)); // qside
		CastleMask[WHITE][1] = (U64(F1) | U64(G1)); // kside
		CastleMask[BLACK][0] = (U64(B8) | U64(C8) | U64(D8));
		CastleMask[BLACK][1] = (U64(F8) | U64(G8));

		return true;
	}

	U64 PseudoAttacksBB(int piece, int square)
	{
		return AttacksBB[piece - 1][square];
	}
}