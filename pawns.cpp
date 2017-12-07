#include <string.h>

#include "pawns.h"
#include "globals.h"
#include "utils.h"

using namespace Globals;

PawnTable pawnTable;

namespace Penalty
{
  float doubledPawn[2] = { -8.0, -12.0 };
  float shelterPawn[2] = { 4.0, 1.0 };
  float isolatedPawn[2] = { -5.0,-10.0 };
  float backwardPawn[2] = { -5.0,-1.0 };
  float chainPawn[2] = { 4.0, 8.0 };
  float passedPawn[2] = { 10.0, 20.0 };
  float semiOpen[2] = { -5.0, -9.0 };
}

// sz for 16 pawns distributed among 48 sqrs. ~ 454,253,679 elts
// sps we only need to store .1% of that ~454254 ~7 mb
PawnTable::PawnTable() : table(0)
{
  init();
};

PawnTable::~PawnTable()
{
  if (table)
    {
      printf("..deleted pawn hash table\n");
      delete[] table;
    }
};

bool PawnTable::init()
{
  sz_kb = 10 * 1024;// opts["PawnHashKB"]; // 4mb
  nb_elts = 1024 * sz_kb / sizeof(PawnEntry);
  nb_elts = nearest_power_of_2(nb_elts);
  nb_elts = nb_elts <= 256 ? 256 : nb_elts;

  if (nb_elts < 256)
    {
      printf("..[PawnTable] failed to set nb of pawn hash table elements, abort. \n");
      return false;
    }

  if (!table && (table = new PawnEntry[nb_elts]()) == 0)
    {
      printf("..[PawnTable] alloc failed\n");
      return false;
    }
  return true;
}

void PawnTable::clear()
{
  memset(table, 0, nb_elts * sizeof(PawnEntry));
}

/*
  basic scheme:
  1. bonus for passed pawns
  2. penalty for doubled, isolated, and backward
  3. connected - pawn chain bonus
  4. king safety structures - shelter bonus
  5. TODO: check this eval 5k2/3p4/2p2p2/1p1nn3/4N3/3N1P2/2P3PP/3K4
*/
PawnEntry * PawnTable::get(Board& b, GamePhase gp)
{
  U64 k = b.pawn_key();
  int idx = k & (nb_elts - 1);
  if (table[idx].key == k)
    {
      return &table[idx];
    }
  else
    {
      table[idx].key = k;
      table[idx].value = eval(b, WHITE, gp, idx) - eval(b, BLACK, gp, idx);
      return &table[idx];
    }
}

int PawnTable::eval(Board& b, Color c, GamePhase gp, int idx)
{
  float base = 0.0;
  U64 w_pawns = b.get_pieces(WHITE, PAWN);
  U64 b_pawns = b.get_pieces(BLACK, PAWN);
  table[idx].passedPawns[c] = 0ULL;
  table[idx].isolatedPawns[c] = 0ULL;
  table[idx].doubledPawns[c] = 0ULL;
  table[idx].backwardPawns[c] = 0ULL;
  table[idx].attacks[c] = 0ULL;
  table[idx].kingPawns[c] = 0ULL;
  table[idx].chainPawns[c] = 0ULL;
  table[idx].undefended[c] = 0ULL;
  table[idx].chainBase[c] = 0ULL;

  int *sqs = b.sq_of<PAWN>(c);
  for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
    {
      // update the attack bitmap
      table[idx].attacks[c] |= PawnAttacksBB[c][from];

      // check if shelter pawn -- a double bonus (see eval in king-safety where a similar bonus is applied).
      if (PseudoAttacksBB(KING, b.sq_of<KING>(c)[1]) & SquareBB[from])
	//if (KingSafetyBB[c][b.king_square(c)] & SquareBB[from])
	{
	  table[idx].kingPawns[c] |= SquareBB[from];
	  base += Penalty::shelterPawn[gp];
	}

      // eval passed pawns
      U64 tmp = PassedPawnBB[c][from] & (c == WHITE ? b_pawns : w_pawns);
      if (!tmp)
	{
	  table[idx].passedPawns[c] |= SquareBB[from];
	  base += Penalty::passedPawn[gp];
	}

      // eval isolated pawns		
      tmp = NeighborColsBB[COL(from)] & (c == WHITE ? w_pawns : b_pawns);
      if (!tmp)
	{
	  table[idx].isolatedPawns[c] |= SquareBB[from];
	  base += Penalty::isolatedPawn[gp];
	}

      // eval backward pawns      
      if (count(tmp) == 2)
	{
	  int sq1 = pop_lsb(tmp);
	  int sq2 = pop_lsb(tmp);
	  if ((c == WHITE ? ROW(sq1) > ROW(from) && ROW(sq2) > ROW(from) :
	       ROW(sq1) < ROW(from) && ROW(sq2) < ROW(from)))
	    {
	      table[idx].backwardPawns[c] |= SquareBB[from];
	      U64 isopen = ColBB[COL(from)] & (w_pawns | b_pawns);
	      if (count(isopen) == 1)
		base += Penalty::backwardPawn[gp];
	    }
	}

      // eval backward pawns with one neighbor instead of 2
      if (count(tmp) == 1)
	{
	  int sq1 = pop_lsb(tmp);
	  if ((c == WHITE ? ROW(sq1) > ROW(from) : ROW(sq1) < ROW(from)))
	    {
	      table[idx].backwardPawns[c] |= SquareBB[from];
	      base += Penalty::backwardPawn[gp];
	    }
	}

      // eval doubled pawns
      tmp = ColBB[COL(from)] & (c == WHITE ? w_pawns : b_pawns);
      if (more_than_one(tmp))
	{
	  table[idx].doubledPawns[c] |= tmp;
	  base += Penalty::doubledPawn[gp];

	  // doubled pawns which are isolated are the "most" weak
	  tmp = NeighborColsBB[COL(from)] & (c == WHITE ? w_pawns : b_pawns);
	  if (!tmp) base += Penalty::isolatedPawn[gp];

	  // doubled pawns on open file
	  //U64 isclosed = ColBB[COL(from)] & (c == WHITE ? b_pawns : w_pawns);
	  //if (isclosed == 0ULL) base += Penalty::isolatedPawn[gp];

	  // doubled king shelter pawns.. :(
	}

      // eval pawns on open files (usually bad)
      tmp = ColBB[COL(from)] & (w_pawns | b_pawns);
      if (count(tmp) == 1)
	{
	  // make sure its "our" pawn and not an enemy pawn on the semi-open file
	  U64 ourpawn = ColBB[COL(from)] & (c == WHITE ? w_pawns : b_pawns);
	  if (ourpawn)
	    {
	      base += Penalty::semiOpen[gp];
	    }
	}
      // chain pawn bonus 
      // 1. do not always want to discourage pawn advances that "break" connectedness, this bonus can be a drawback sometimes ... 
      // 2. seems to "double" count..we have already penalized for "dbouled, isolated" pawns, now give bonus if not double/isolated ? ...
      // 3. favors longer pawn chains      
      tmp = PawnAttacksBB[c == WHITE ? BLACK : WHITE][from] & (c == WHITE ? w_pawns : b_pawns);
      if (tmp)
	{
	  table[idx].chainPawns[c] |= (tmp | SquareBB[from]);
	  int nb = count(tmp) / 2;
	  if (nb > 2) base += nb * Penalty::chainPawn[gp];
	}
      else
	{
	  table[idx].chainBase[c] |= SquareBB[from];  // store the base of the pawn chain
	}
      // compute those pawns whiceh are "hanging" 
      //U64 defenders = b.attackers_of(from) & b.colored_pieces(c);
      //if (!defenders) table[idx].undefended[c] |= SquareBB[from];


    } // end loop over sqs

  return (int)(base * 0.5);
}
