#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#include "magic.h"
#include "globals.h"
#include "random.h"

using namespace Magic;

MagicTable RMagics[SQUARES];
MagicTable BMagics[SQUARES];

U64 rook_atks[4900]; // 39 kb, 4900 is the sum of all unique attack arrays for the rook (see r_offset below)
U64 bish_atks[1428]; // 11 kb, 1428 is the sum of all unique attack arrays for the bishop (see b_offset below)

// 4096 is computed from counting the number of possible blockers for a rook/bishop at a given square.
// E.g. the rook@A1 has 12 squares which can be blocked (A8,H1 have been removed)
// in mathematica : sum[12!/(n!*(12-n)!),{n,1,12}] = 4095 .. similar computation for bishop@E4.
U8 r_occ[SQUARES][4096];
U8 b_occ[SQUARES][512];

// these offsets tally the number of unique attack arrays
// that exist for bishop/rook at the given square.  E.g. all values
// for the rook can be computed by 
// value = empty_row1_sqs * empty_row2_sqs * empty_col1_sqs * empty_col2_sqs
// notice rook@A1 has 49 unique attack patterns (but 4096 occupancies at A1)

int b_offset[SQUARES] =
  {
    7,  6,  10,  12,  12,  10,  6,  7,
    6,  6,  10,  12,  12,  10,  6,  6,
    10, 10,  40,  48,  48,  40, 10, 10,
    12, 12,  48, 108, 108,  48, 12, 12,
    12, 12,  48, 108, 108,  48, 12, 12,
    10, 10,  40,  48,  48,  40, 10, 10,
    6,  6,  10,  12,  12,  10,  6,  6,
    7,  6,  10,  12,  12,  10,  6,  7
  };

int r_offset[SQUARES] =
  {
    49,  42,  70,  84,  84,  70,  42,  49,
    42,  36,  60,  72,  72,  60,  36,  42,
    70,  60, 100, 120, 120, 100,  60,  70,
    84,  72, 120, 144, 144, 120,  72,  84,
    84,  72, 120, 144, 144, 120,  72,  84,
    70,  60, 100, 120, 120, 100,  60,  70,
    42,  36,  60,  72,  72,  60,  36,  42,
    49,  42,  70,  84,  84,  70,  42,  49
  };


Rand r;
void Magic::init()
{
  //Clock c;
  U64 occupancy[4096], atks[4096], used[4096], mask, b, filter;
  int occ_size, idx;
  //c.start();

  for (int p = BISHOP; p <= ROOK; ++p)
    {
      // the number of bits to set in a random 64-bit number
      // during random number generation (generally tried to keep this small)
      int bits = (p == ROOK ? 9 : 8);

      for (int s = A1; s <= H8; ++s)
	{
	  mask = (p == ROOK ? Globals::RookMask[s] : Globals::BishopMask[s]);

	  int shift = 64 - count(mask);
	  b = 0ULL;
	  occ_size = 0;
	  U64 magic;

	  // enumerate all the occupancy combinations of the bishop/rook mask
	  // defined above.  The attack_bm() routine returns set bits up until the first
	  // blocking bit is found along each row/col/diagonal.
	  do
	    {
	      occupancy[occ_size] = b;
	      atks[occ_size++] = attack_bm(p, s, b);
	      b = (b - mask) & mask;
	    } while (b);

			

	  // main search for a candidate magic multiplier for the move hashing.
	  // The filtering removes the upper-most set bits after multiplication,
	  // we also enforce that the total number of bits < 8.
	  int i = 0; // used as a flag to determine if the magic multiplier passes
	  do
	    {
				
	      do
		{
		  magic = next_rand(bits);
		  filter = (magic * mask) & 0xFFFF000000000000;
		} while (count(filter) < 8);

	      memset(used, 0, occ_size * sizeof(U64));


	      // main test of the magic multiplier computed above. The multiplier 
	      // passes if it maps each occupancy bitboard to a unique index
	      // stored in the "used" array.
	      for (i = 0; i < occ_size; ++i)
		{
		  idx = index(magic, mask, occupancy[i], shift);

		  if ((used[idx] == 0ULL && (used[idx] != occupancy[i] || occupancy[i] == 0ULL)))
		    {
		      used[idx] = occupancy[i];
		    }
		  else break;
		}
				
	    } while (i != occ_size);

	  // dbg...
	  //printf("p = %d, s = %d, magic = %lu \t iter = %d  t = %g(ms)\n",p,s,magic,counter,c.ms());


	  // idea is to reduce the size of the stored arrays by removing redundant occupancies
	  // from the hashing.  Redundant occupancies occur when there are blockers behind blockers
	  // we always only care about the first blocker in along the row/col/diagonal.
	  // Below we will store the b_occ/r_occ (array of ints) vs. array of U64[4096] occupancies.

	  int offset = 0;
	  for (int k = 0; k<s; ++k) offset += (p == ROOK ? r_offset[k] : b_offset[k]);
			
	  U64 temp_storage[SQUARES][144];
	  memset(temp_storage, 0, 64 * 144 * sizeof(U64));

	  // this loop filters those occupancies which are redundant, the index : r_occ[s][idx] + offset
	  // will point to the correct attack array for multiple kinds of non-unique occupancies 
	  for (i = 0; i < occ_size; ++i)
	    {
	      idx = index(magic, mask, occupancy[i], shift);

	      for (int j = 0; j<144; ++j)
		{
		  if (!temp_storage[s][j] && temp_storage[s][j] != atks[i])
		    {
		      temp_storage[s][j] = atks[i];
		      (p == ROOK ? r_occ[s][idx] = j : b_occ[s][idx] = j);
		      break;
		    }
		  else if (temp_storage[s][j] == atks[i])
		    {
		      (p == ROOK ? r_occ[s][idx] = j : b_occ[s][idx] = j);
		      break;
		    }
		}

	      (p == ROOK ? rook_atks[r_occ[s][idx] + offset] = atks[i] :
	       bish_atks[b_occ[s][idx] + offset] = atks[i]);
	    }

	  // the magic passed all tests, final step is to save all magic-related data
	  // in globally accessible structs.
	  if (p == BISHOP)
	    {
	      BMagics[s].index_occ = b_occ[s];
	      BMagics[s].attacks = bish_atks;
	      BMagics[s].magic = magic;
	      BMagics[s].mask = mask;
	      BMagics[s].shift = shift;
	      BMagics[s].offset = offset;
	    }
	  else
	    {
	      RMagics[s].index_occ = r_occ[s];
	      RMagics[s].attacks = rook_atks;
	      RMagics[s].magic = magic;
	      RMagics[s].mask = mask;
	      RMagics[s].shift = shift;
	      RMagics[s].offset = offset;
	    }
	}
    }
  //c.print("..generate magics");
}

// this method scans over all attacks for all occupancies at each square
// for the bishop/rook and checks that the magic multipliers return an attack
// array which agrees with that returned by the attack_bm() method, if there 
// is a discrepency, we abort at start (we won't be able to search any positions anyway in that case)
bool Magic::check_magics()
{
  U64 occupancy[4096], atks[4096], mask, b;
  int size, i;

  for (int p = BISHOP; p <= ROOK; p++)
    {
      for (int s = A1; s <= H8; s++)
	{
	  mask = (p == ROOK ? Globals::RookMask[s] : Globals::BishopMask[s]);
	  b = 0ULL;
	  size = 0;

	  do
	    {
	      occupancy[size] = b;
	      atks[size++] = attack_bm(p, s, b);
	      b = (b - mask) & mask;
	    } while (b);

	  for (i = 0; i < size; i++)
	    {
	      U64 mvs = (p == ROOK ? attacks<ROOK>(occupancy[i], s) : attacks<BISHOP>(occupancy[i], s));

	      if ((mvs - atks[i]) != 0ULL)
		{
		  printf("..attack hash returned corrupted entry, abort!\n");
		  printf("---- occupancy ----\n");
		  display(occupancy[i]);

		  printf("---- correct attack array ----\n");
		  display(mvs);

		  printf("---- hashed attack array ----\n");
		  display(atks[i]);
		  return false;
		}

	    }
	}
    }

  //printf("..PASSED!\n");
  return true;
}

U64 Magic::next_rand(int bits)
{
  U64 rm = 0ULL;
  for (int i = 0; i < bits; ++i)
    {
      rm |= Globals::SquareBB[(r.rand() & 63)];
    }
  return rm;
}

U64 Magic::attack_bm(int p, int s, U64 block)
{
  int rsteps[4] = { -1, 1, -8, 8 };
  int bsteps[4] = { -7, 7, -9, 9 };
  U64 bm = 0ULL;

  for (int j = 0; j<4; ++j)
    {
      U64 tmp = 0ULL;
      int steps = 1;

      while (!(tmp & block))
	{
	  if (steps == 8) break;
			
	  int to = (p == ROOK ? s + (steps++)*rsteps[j] : s + (steps++)*bsteps[j]);

	  if (on_board(to) && col_distance(s, to) <= 7 && row_distance(s, to) <= 7 &&
	      ( (p == BISHOP && on_diagonal(s, to)) || (p == ROOK && (same_row(s, to) || same_col(s, to))) ))
	    {
	      tmp |= Globals::SquareBB[to];
	    }
	}
		
      bm |= tmp;
    }
  return bm;
}
