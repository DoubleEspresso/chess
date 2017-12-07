#include "zobrist.h"
#include "random.h"

namespace Zobrist
{
	U64 zobrist_rands[SQUARES][2][PIECES];
	U64 zobrist_cr[2][16];
	U64 zobrist_col[8];
	U64 zobrist_stm[2];
	U64 zobrist_move50[50];

	bool init()
	{
		Rand rand(6364136223846793005ULL);

		try
		{
			for (int s = 0; s < SQUARES; ++s)
			{
				for (int c = WHITE; c <= BLACK; ++c)
				{
					for (int p = PAWN; p <= KING; ++p)
					{
						zobrist_rands[s][c][p] = rand.rand64();
					}
				}
			}
			// castle rights
			for (int c = WHITE; c <= BLACK; ++c)
			{
				for (int r = 0; r < 16; ++r)
				{
					zobrist_cr[c][r] = rand.rand64();
				}
			}
			// ep-randoms
			for (int c = 0; c < COLS; ++c)
			{
				zobrist_col[c] = rand.rand64();
			}
			// stm-randoms
			for (int c = WHITE; c <= BLACK; ++c)
			{
				zobrist_stm[c] = rand.rand64();
			}
			// move50 rands
			for (int m = 0; m < 50; ++m)
			{
				zobrist_move50[m] = rand.rand64();
			}
		}
		catch (const std::exception& e)
		{
			printf("..[Zobrist] Exception building zobrist keys: %s", e.what());
			return false;
		}
		return true;
	}

	U64 piece_rands(const U8& square, const U8& color, const U8& piece)
	{
		return zobrist_rands[square][color][piece];
	}

	U64 castle_rands(const U8& color, const U16 &castle_rights)
	{
		return zobrist_cr[color][castle_rights];
	}

	U64  ep_rands(const U8& column)
	{
		return zobrist_col[column];
	}

	U64 stm_rands(const U8& c)
	{
		return zobrist_stm[c];
	}

	U64 mv50_rands(const U8& mv)
	{
		return zobrist_move50[mv];
	}
}
