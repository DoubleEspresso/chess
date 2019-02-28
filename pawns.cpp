#include <vector>

#include "pawns.h"
#include "types.h"
#include "utils.h"
#include "bitboards.h"
#include "squares.h"

pawn_table ptable;

template<Color c>
int16 evaluate(const position& p, pawn_entry& e);

inline size_t pow2(size_t x) {
  return x <= 2 ? x : pow2(x >> 1) << 1;
}

pawn_table::pawn_table() : sz_kb(0), count(0) {
  init();
}



void pawn_table::init() {
  sz_kb = 10 * 1024; // todo : input mb parameter
  count = 1024 * sz_kb / sizeof(pawn_entry);
  count = pow2(count);
  count = (count < 1024 ? 1024 : count);
  entries = std::unique_ptr<pawn_entry[]>(new pawn_entry[count]());
  clear();
}



void pawn_table::clear() {
  memset(entries.get(), 0, count * sizeof(pawn_entry));
}



pawn_entry * pawn_table::fetch(const position& p) {
  U64 k = p.pawnkey();
  unsigned idx = k & (count - 1);
  if (entries[idx].key == k) {
    return &entries[idx];
  }
  else {
    entries[idx] = {};
    entries[idx].key = k;
    entries[idx].val = evaluate<white>(p, entries[idx]) - evaluate<black>(p, entries[idx]);
    return &entries[idx];
  }
}




template<Color c>
int16 evaluate(const position& p, pawn_entry& e) {

  // sq score scale factors by column
  std::vector<float> pawn_scaling { 0.86, 0.90, 0.95, 1.00, 1.00, 0.95, 0.90, 0.86 };
  std::vector<float> material_vals { 100.0, 300.0 , 315.0, 480.0, 910.0 };
  
  Color them = Color(c ^ 1);

  U64 pawns = p.get_pieces<c, pawn>(); 
  U64 epawns = them == white ?
    p.get_pieces<white, pawn>() :
    p.get_pieces<black, pawn>();
  
  Square * sqs = p.squares_of<c, pawn>();

  Square ksq = p.king_square(c);

  int16 score = 0;
  
  for (Square s = *sqs; s != no_square; s = *++sqs) {

    U64 fbb = bitboards::squares[s];

    score += square_score<c>(pawn, Square(s));
    score += pawn_scaling[util::col(s)] * material_vals[pawn];

    
    // pawn attacks
    e.attacks[c] |= bitboards::pattks[c][s];

    // king shelter
    if ((bitboards::kmask[ksq] & fbb)) {
      e.king[c] |= fbb;
    }
    
    // passed pawns
    U64 mask = bitboards::passpawn_mask[c][s] & epawns;  
    if (mask == 0ULL) { e.passed[c] |= fbb; }

    // isolated pawns
    U64 neighbors_bb = bitboards::neighbor_cols[util::col(s)] & pawns;
    if (neighbors_bb == 0ULL) {
      e.isolated[c] |= fbb;
    }

    // backward

    // pawns by square color
    U64 wsq = bitboards::colored_sqs[white] & fbb;
    if (wsq) e.light[c] |= fbb;
    U64 bsq = bitboards::colored_sqs[black] & fbb;
    if (bsq) e.dark[c] |= fbb;

    // doubled pawns
    U64 doubled = bitboards::col[util::col(s)] & pawns;
    if (bits::more_than_one(doubled)) {
      e.doubled[c] |= doubled;

      // isolated and doubled
      //doubled = bitboards::neighbor_cols[util::col(s)] & pawns;
      // doubled == 0ULL // ...
    }

    // semi-open file pawns
    // pawn islands
    // pawn chain tips
    // pawn chain bases

  }
  
  return score;  
}
