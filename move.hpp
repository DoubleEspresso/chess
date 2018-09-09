#include "move.h"
#include "position.h"
#include "magics.h"
#include "bits.h"
#include "bitboards.h"

using namespace bitboards;
using namespace bits;

// utilities
namespace {
  template<Dir> inline void shift(U64& b);  
  template<> inline void shift<N>(U64& b)  { b <<= 8; }
  template<> inline void shift<S>(U64& b)  { b >>= 8; }
  template<> inline void shift<NN>(U64& b) { b <<= 16; }
  template<> inline void shift<SS>(U64& b) { b >>= 16; }
  template<> inline void shift<NW>(U64& b) { b <<= 7; }
  template<> inline void shift<NE>(U64& b) { b <<= 9; }
  template<> inline void shift<SW>(U64& b) { b >>= 7; }
  template<> inline void shift<SE>(U64& b) { b >>= 9; }  
}


template<Movetype mt, Piece p, Color c>
inline void movegen<mt, p, c>::encode(U64& b, const int& f) {  
  while (b) list[last++].m = (f | (pop_lsb(b) << 6) | (mt << 12));
}

template<Movetype mt, Piece p, Color c>
inline void movegen<mt, p, c>::encode_pawn_pushes(U64& b, const int& dir) {
  while (b) {
    int to = pop_lsb(b);
    list[last++].m = ((to + dir) | (to << 6) | (mt << 12)); 
  }
}

template<Movetype mt, Piece p, Color c>
inline void movegen<mt, p, c>::encode_promotions(U64& b, const int& f) {
  while (b) {
    int to = pop_lsb(b);
    list[last++].m = (f | (pop_lsb(b) << 6) | (mt << 12));     // queen
    list[last++].m = (f | (pop_lsb(b) << 6) | ((mt+2) << 12)); // rook
    list[last++].m = (f | (pop_lsb(b) << 6) | ((mt+4) << 12)); // bishop
    list[last++].m = (f | (pop_lsb(b) << 6) | ((mt+6) << 12)); // knight   
  }
}

template<Movetype mt, Piece p, Color c> void movegen<mt, p, c>::print() {
  for(int j=it; j<last; ++j) {    
    std::cout << list[j].to_string() << " ";
  }
  std::cout << "\n";
}


//------------------------------
// pawn moves
//------------------------------
template<>
void movegen<quiet, pawn, white>::generate(const position& p) {
  
  U64 empty = ~(p.all_pieces());
  U64 pawns = p.get_pieces<white, pawn>();
  U64 single_pushes = pawns & pawnmask[white]; // filter the promotion candidates
  U64 double_pushes = pawns & bitboards::row[r2];

  shift<N>(single_pushes);
  single_pushes &= empty;

  if (single_pushes != 0ULL) encode_pawn_pushes(single_pushes, -8);
  
  for (int i=0; i<2; ++i) {
    shift<N>(double_pushes);
    double_pushes &= empty;
  }

  if (double_pushes != 0ULL) encode_pawn_pushes(double_pushes, -16);
}

