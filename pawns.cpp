#include <string.h>

#include "pawns.h"
#include "globals.h"
#include "utils.h"

using namespace Globals;

PawnTable pawnTable;

namespace Penalty {
  float doubledPawn[2] = { -8.0, -12.0 };
  float shelterPawn[2] = { 4.0, 1.0 };
  float isolatedPawn[2] = { -5.0,-10.0 };
  float backwardPawn[2] = { -5.0,-1.0 };
  float chainPawn[2] = { 4.0, 8.0 };
  float passedPawn[2] = { 10.0, 20.0 };
  float semiOpen[2] = { -5.0,-9.0 };
}

// sz for 16 pawns distributed among 48 sqrs. ~ 454,253,679 elts
// sps we only need to store .1% of that ~454254 ~7 mb
PawnTable::PawnTable() : table(0) {
  init();
}

PawnTable::~PawnTable() {
  if (table) {
    printf("..deleted pawn hash table\n");
    delete[] table;
  }
}

bool PawnTable::init() {
  sz_kb = 10 * 1024;// opts["PawnHashKB"]; // 4mb
  nb_elts = 1024 * sz_kb / sizeof(PawnEntry);
  nb_elts = nearest_power_of_2(nb_elts);
  nb_elts = nb_elts <= 256 ? 256 : nb_elts;

  if (nb_elts < 256) {
    printf("..[PawnTable] failed to set nb of pawn hash table elements, abort. \n");
    return false;
  }

  if (!table && (table = new PawnEntry[nb_elts]()) == 0) {
    printf("..[PawnTable] alloc failed\n");
    return false;
  }
  return true;
}

void PawnTable::clear() {
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
PawnEntry * PawnTable::get(Board& b, GamePhase gp) {
  U64 k = b.pawn_key();
  int idx = k & (nb_elts - 1);
  if (table[idx].key == k) {
      return &table[idx];
    }
  else {
      table[idx].key = k;
      table[idx].value = eval(b, WHITE, gp, idx) - eval(b, BLACK, gp, idx);
      return &table[idx];
    }
}

int PawnTable::eval(Board& b, Color c, GamePhase gp, int idx) {
  float base = 0.0;
  U64 our_pawns = (c == WHITE ? b.get_pieces(WHITE, PAWN) : b.get_pieces(BLACK, PAWN));
  U64 their_pawns = (c == WHITE ? b.get_pieces(BLACK, PAWN) : b.get_pieces(WHITE, PAWN));
  U64 all_pawns = our_pawns | their_pawns;
  Color them = (c == WHITE ? BLACK : WHITE);
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
  for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs) {
    U64 fbb = SquareBB[from];
    
      // store the pawn attack bitmap
      table[idx].attacks[c] |= PawnAttacksBB[c][from];

      // king shelter pawn
      if (PseudoAttacksBB(KING, b.sq_of<KING>(c)[1]) & fbb) {
	table[idx].kingPawns[c] |= fbb;
	base += Penalty::shelterPawn[gp];
      }

      // passed pawns
      U64 tmp = PassedPawnBB[c][from] & their_pawns;
      if (empty(tmp)) {
	table[idx].passedPawns[c] |= fbb;
	base += Penalty::passedPawn[gp];
      }

      // isolated pawns		
      tmp = NeighborColsBB[COL(from)] & our_pawns;
      if (empty(tmp)) {
	table[idx].isolatedPawns[c] |= fbb;
	base += Penalty::isolatedPawn[gp];
      }

      // backward pawns      
      if (count(tmp) >= 2) {
	int sq1 = pop_lsb(tmp);
	int sq2 = pop_lsb(tmp);
	if ((c == WHITE ? ROW(sq1) >= ROW(from) && ROW(sq2) >= ROW(from) :
	     ROW(sq1) <= ROW(from) && ROW(sq2) <= ROW(from))) {
	  table[idx].backwardPawns[c] |= fbb;
	  U64 isopen = ColBB[COL(from)] & all_pawns;
	  if (count(isopen) == 1)
	    base += Penalty::backwardPawn[gp];
	}
      }

      // backward pawns with one neighbor instead of 2
      if (count(tmp) == 1) {
	int sq1 = pop_lsb(tmp);
	if ((c == WHITE ? ROW(sq1) >= ROW(from) : ROW(sq1) <= ROW(from))) {
	  table[idx].backwardPawns[c] |= fbb;
	  base += Penalty::backwardPawn[gp];
	}
      }

      // doubled pawns
      tmp = ColBB[COL(from)] & our_pawns;
      if (more_than_one(tmp)) {
	table[idx].doubledPawns[c] |= tmp;
	base += Penalty::doubledPawn[gp];

	// ..and isolated
	tmp = NeighborColsBB[COL(from)] & our_pawns;
	if (empty(tmp)) base += Penalty::isolatedPawn[gp];
      }

      // pawns on open files      
      tmp = ColBB[COL(from)] & all_pawns;
      if (count(tmp) == 1) {
	U64 ourpawn = ColBB[COL(from)] & our_pawns;
	if (!empty(ourpawn)) base += Penalty::semiOpen[gp];
      }
      
      // chain pawns (track chain-base)
      tmp = PawnAttacksBB[them][from] & our_pawns;
      if (!empty(tmp)) {
	table[idx].chainPawns[c] |= (tmp | fbb);
      }
      else table[idx].chainBase[c] |= fbb;      
    }
  
  return (int)(base * 0.65);
}
