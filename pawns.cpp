#include <vector>

#include "pawns.h"
#include "types.h"
#include "utils.h"
#include "bitboards.h"


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

  /*
  Color them = Color(c ^ 1);

  U64 pawns = p.get_pieces<c, pawn>(); 

  Square * sqs = p.squares_of<c, pawn>();

  Square ksq = p.king_square(c);


  
  for (Square s = *sqs; s != no_square; s = *++sqs) {

    U64 fbb = bitboards::squares[s];

    // pawn attacks
    e.attacks[c] |= bitboards::pattks[c][s];

    // king shelter
    if ((bitboards::kmask[ksq] & fbb)) {
      e.king[c] |= fbb;
    }
    
  }
  */
  return 0;  
}
