#include <string.h>

#include "pawns.h"
#include "globals.h"
#include "utils.h"
#include "opts.h"

using namespace Globals;

PawnTable pawnTable;

namespace Penalty
{
  float doubledPawn[2] = { -6.0, -8.0 };
  float shelterPawn[2] = { 2.0, 1.0 };
  float isolatedPawn[2] = { -4.0,-6.0 };
  float backwardPawn[2] = { -2.0,-4.0 };
  float chainPawn[2] = { 2.0, 4.0 };
  float passedPawn[2] = { 4.0, 10.0 };
  float semiOpen[2] = { -3.0,-8.0 };
}

// sz for 16 pawns distributed among 48 sqrs. ~ 454,253,679 elts
// sps we only need to store .1% of that ~454254 ~7 mb
PawnTable::PawnTable() : table(0)
{
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
  sz_kb = options["pawn table size mb"] * 1024; // 4mb
  nb_elts = 1024 * sz_kb / sizeof(PawnEntry);
  nb_elts = nearest_power_of_2(nb_elts);
  nb_elts = nb_elts <= 256 ? 256 : nb_elts;
	
  if (nb_elts < 256)
    {
      printf("..failed to set nb of pawn hash table elements, abort. \n");
      return false;
    }

  if (!table && (table = new PawnEntry[nb_elts]()))
    {
      //printf("..initialized pawn hash table ok : size %3.1f kb, %lu elts.\n", float(nb_elts)*float(sizeof(PawnEntry)) / float(1024.0), nb_elts);
    }
  else
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
  table[idx].semiOpen[c] = 0ULL;
  table[idx].pawnChainTips[c] = 0ULL;
  table[idx].blockedCenter = 0;
  table[idx].kSidePawnStorm = 0;
  table[idx].qSidePawnStorm = 0;
  
  bool foundPawnChainTip = false;
  int maxRowPawnSq = (c == WHITE ? 0 : 63);
  int centerPawnsLocked = 0;
  int nbKsidePawnStorm = 0;
  int nbQsidePawnStorm = 0;

  int *sqs = b.sq_of<PAWN>(c);
  for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
    {
      // update the attack bitmap
      table[idx].attacks[c] |= PawnAttacksBB[c][from];

      // check if shelter pawn -- a double bonus (see eval in king-safety where a similar bonus is applied).
      if (PseudoAttacksBB(KING, b.sq_of<KING>(c)[1]) & SquareBB[from])
	{
	  table[idx].kingPawns[c] |= SquareBB[from];
	  base += Penalty::shelterPawn[gp];
	}

      // eval passed pawns
      U64 tmp = PassedPawnBB[c][from] & (c == WHITE ? b_pawns : w_pawns);
      if (!tmp) // no enemy pawns block this pawn
	{
	  table[idx].passedPawns[c] |= SquareBB[from];
	  base += Penalty::passedPawn[gp];

	  U64 infrontBB = SpaceInFrontBB[c][from];
	  int dist = count(infrontBB);
	  if (dist < 4) base += (gp == MIDDLE_GAME ? 4 : 16);
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
	      U64 isopen = ColBB[COL(from)] & (w_pawns | b_pawns); // check backward pawn on open file
	      if (count(isopen) == 1)
		base += Penalty::backwardPawn[gp];
	    }
	}

      // eval backward pawns with one neighbor instead of 2, hanging pawns
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
	}

      // eval pawns on open files (usually bad)
      tmp = ColBB[COL(from)] & (w_pawns | b_pawns);
      if (count(tmp) == 1)
	{
	  table[idx].semiOpen[c] |= SquareBB[from]; // store as potential targets
	  // make sure its "our" pawn and not an enemy pawn on the semi-open file
	  U64 ourpawn = ColBB[COL(from)] & (c == WHITE ? w_pawns : b_pawns);
	  if (ourpawn)
	    {
	      base += Penalty::semiOpen[gp];
	    }
	}
      // chain pawns
      tmp = PawnAttacksBB[c == WHITE ? BLACK : WHITE][from] & (c == WHITE ? w_pawns : b_pawns);
      if (tmp)
	{
	  int row = ROW(from);
	  if (c == WHITE && ROW(from) > ROW(maxRowPawnSq)) maxRowPawnSq = from;
	  if (c == BLACK && ROW(from) < ROW(maxRowPawnSq)) maxRowPawnSq = from;

	  table[idx].chainPawns[c] |= (tmp | SquareBB[from]);
	  int nb = count(tmp) / 2;
	  base +=  nb * Penalty::chainPawn[gp];
	  if (nb > 2) base += nb * Penalty::chainPawn[gp];
	}
      else if (PawnAttacksBB[c][from] & (c == WHITE ? w_pawns : b_pawns))
	{
	  table[idx].chainBase[c] |= SquareBB[from];  // store the base of the pawn chain
	}

      // determine/store structure type
      // 1. locked center pawns/open center
      // 2. pawn storms
      if (SquareBB[from] & BigCenterBB)
	{
	  int infront = (c == WHITE ? from + 8 : from - 8);
	  if (b.color_on(infront) == (c == WHITE ? BLACK : WHITE) && b.piece_on(infront) == PAWN)
	    {
	      ++centerPawnsLocked;
	    }
	}
      if (SquareBB[from] & kSidePawnStormBB)
	{
	  ++nbKsidePawnStorm;
	}
      if (SquareBB[from] & qSidePawnStormBB)
	{
	  ++nbQsidePawnStorm;
	}

      
      // compute those pawns which are "hanging" 
      U64 defenders = b.attackers_of(from) & b.colored_pieces(c);
      if (!defenders) table[idx].undefended[c] |= SquareBB[from];
    } // end loop over sqs

	  
  if ((ROW(maxRowPawnSq) > 1 && c == WHITE) || ROW(maxRowPawnSq) < 6 && c == BLACK)
    {
      table[idx].pawnChainTips[c] |= SquareBB[maxRowPawnSq];
    }

  // set position type data
  if (centerPawnsLocked >= 2) table[idx].blockedCenter = 1;  
  if (nbKsidePawnStorm >= 3) table[idx].kSidePawnStorm = 1;
  if (nbQsidePawnStorm >= 3) table[idx].qSidePawnStorm = 1;
  
  return (int)base;
}


void PawnTable::debug(PawnEntry& e)
{
  for (int c=WHITE; c <= BLACK; ++c)
    { 
      std::string cs = (c == WHITE ? "white" : "black");
      printf("..doubled pawns (%s)\n",cs.c_str());
      display(e.doubledPawns[c]);
    }

  for (int c=WHITE; c <= BLACK; ++c)
    {
      std::string cs = (c == WHITE ? "white" : "black");
      printf("..isolated pawns (%s)\n",cs.c_str());
      display(e.isolatedPawns[c]);
    }

  for (int c=WHITE; c <= BLACK; ++c)
    {
      std::string cs = (c == WHITE ? "white" : "black");
      printf(".. backward pawns (%s)\n",cs.c_str());
      display(e.backwardPawns[c]);
    }

  for (int c=WHITE; c <= BLACK; ++c)
    {
      std::string cs = (c == WHITE ? "white" : "black");
      printf("..chain pawns (%s)\n",cs.c_str());
      display(e.chainPawns[c]);
    }

  for (int c=WHITE; c <= BLACK; ++c)
    {
      std::string cs = (c == WHITE ? "white" : "black");
      printf("..passed pawns (%s)\n",cs.c_str());
      display(e.passedPawns[c]);
    }

  for (int c=WHITE; c <= BLACK; ++c)
    {
      std::string cs = (c == WHITE ? "white" : "black");
      printf("..king pawns (%s)\n",cs.c_str());
      display(e.kingPawns[c]);
    }

  for (int c=WHITE; c <= BLACK; ++c)
    {
      std::string cs = (c == WHITE ? "white" : "black");
      printf("..attacks from pawns (%s)\n",cs.c_str());
      display(e.attacks[c]);
    }

  for (int c=WHITE; c <= BLACK; ++c)
    {
      std::string cs = (c == WHITE ? "white" : "black");
      printf("..undefended pawns (%s)\n",cs.c_str());
      display(e.undefended[c]);
    }

  for (int c=WHITE; c <= BLACK; ++c)
    {
      std::string cs = (c == WHITE ? "white" : "black");
      printf("..chainbase pawns (%s)\n",cs.c_str());
      display(e.chainBase[c]);      
    }

  printf("...final pawn eval = %d\n", e.value);
}
