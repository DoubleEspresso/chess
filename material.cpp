#include <string.h>

#include "opts.h"
#include "material.h"
#include "board.h"

MaterialTable material;

namespace
{
  // local arrays containing the piece values in middle/end game
  // phase
  int piece_vals_mg[5] = { PawnValueMG, KnightValueMG, BishopValueMG, RookValueMG, QueenValueMG };
  int piece_vals_eg[5] = { PawnValueEG, KnightValueEG, BishopValueEG, RookValueEG, QueenValueEG };
  
  // weight factors
/*  float bpWeightMG = opts["bishop pair weight mg"];
  float bpWeightEG = opts["bishop pair weight eg"];
  float rrWeightMG = opts["rook redundancy weight mg"];
  float rrWeightEG = opts["rook redundancy weight eg"];
  float qrWeightMG = opts["queen redundancy weight mg"];
  float qrWeightEG = opts["queen redundancy weight eg"];
  float bishopPairWeightMG = opts["bishop pair weight mg"];
  float bishopPairWeightEG = opts["bishop pair weight eg"];

  float weights_mg[4] = { bpWeightMG, 1.50, rrWeightMG, qrWeightMG };
  float weights_eg[4] = { bpWeightEG, 3.00, rrWeightEG, qrWeightEG };*/

  float weights_mg[4] = { 16, 1.50, 3, 0.5 };
  float weights_eg[4] = { 20, 3.00, 1.15, 0.25 };
}

MaterialTable::MaterialTable() : table_size(0), table(0) { };

MaterialTable::~MaterialTable()
{
  if (table)
    {
      printf("..[MaterialTable] deleting table\n"); 
      delete[] table; table = 0;
    }
};

// main wrapper to material values defined in definitions.h
int MaterialTable::material_value(int piece, int gamephase)
{
  return gamephase == MIDDLE_GAME ? piece_vals_mg[piece] : piece_vals_eg[piece];
}

bool MaterialTable::init()
{
  int sz_kb = options["material table size mb"] * 1024;
  nb_elts = 1024 * sz_kb / sizeof(MaterialEntry);

  nb_elts = nearest_power_of_2(nb_elts);
  nb_elts = nb_elts <= 256 ? 256 : nb_elts;

  if (!table && (table = new MaterialEntry[nb_elts]()))
    {
      //float out_sz = float(float(nb_elts*sizeof(MaterialEntry)) / float(1024 * 1024));
      //printf("..allocated material table of %lu elements --> size %3.1f mb.\n", nb_elts, out_sz);
    }
  else
    {
      float out_sz = float(float(nb_elts*sizeof(MaterialEntry)) / float(1024 * 1024));
      printf("..failed to allocate material table of %lu elements --> size %3.1f mb.\n", nb_elts, out_sz);
      return false;
    }
  return true;
}

void MaterialTable::clear()
{
  memset(table, 0, nb_elts * sizeof(MaterialEntry));
}

// main storage for material entries during the search.
MaterialEntry * MaterialTable::get(Board& b)
{
  U64 k = b.material_key();
  int * piece_diffs = b.piece_deltas();
  int * piece_count_w = b.piece_counts(WHITE);
  int * piece_count_b = b.piece_counts(BLACK);

  // the lookup index
  int idx = k & (nb_elts - 1);

  // initialize the base material score
  float base = 0.0;
  float piece_count = 0.0;


  if (table[idx].key == k) return &table[idx];
  else
    {
      table[idx].key = k;
      for (int pt = 1; pt<PIECES - 1; ++pt)
	{
	  piece_count += (piece_count_w[pt] + piece_count_b[pt]);			
	}

      // game phase
      table[idx].game_phase = (piece_count >= 6 ? MIDDLE_GAME : END_GAME);
      int phase = table[idx].game_phase;

      // compute the base material score (just a counting of each piece type multiplied by the value definition).
      for (int pt = 0; pt < PIECES - 1; ++pt)
	{
	  //std::cout << " .. dbg pt=" << SanPiece[pt] << " diff=" << piece_diffs[pt] << " delta value="<< (phase == MIDDLE_GAME ? piece_vals_mg[pt] : piece_vals_eg[pt]) * piece_diffs[pt] <<  std::endl;
	  base += (phase == MIDDLE_GAME ? piece_vals_mg[pt] : piece_vals_eg[pt]) * piece_diffs[pt]; // already symmetical
																									  
	}

      // TODO : evaluate for specific endgames.
      // Material evaluation scheme -- adjust the score based on 3 principles
      // 1. encourage keeping bishop pair
      // 2. encourage to trade when up material
      // 3. rook/queen redundant penalties according GM Larry Kaufman's "principle of the redundancy of major pieces"
      float w1 = (phase == MIDDLE_GAME ? weights_mg[0] : weights_eg[0]);
      //float w2 = (phase == MIDDLE_GAME ? weights_mg[1] : weights_eg[1]);
      float w3 = (phase == MIDDLE_GAME ? weights_mg[2] : weights_eg[2]);
      float w4 = (phase == MIDDLE_GAME ? weights_mg[3] : weights_eg[3]);

      // corrections
      float corr = 0.0;

      // 1. encourage keeping bishop pair
      corr += w1 * piece_diffs[BISHOP];

      //// 2. encourage to trade when up material, in endgame, there are fewer pieces by default and
      //// we don't necessarily want to trade there
      //if (base != 0 && piece_count > 0 && phase != END_GAME)
      //{
      //	corr += w2 * 1 / piece_count * base;
      //}

      // 3. rook/queen redundant penalties according GM Larry Kaufman's "principle of the redundancy of major pieces"
      //corr -= w3 * (piece_count_w[ROOK] - piece_count_b[ROOK]);
      //corr -= w4 * (piece_count_w[QUEEN] - piece_count_b[QUEEN]);
		
      table[idx].value =  (int)(base + corr);
    }
  return &table[idx];
}

