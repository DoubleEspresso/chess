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
inline void movegen<mt, p, c>::encode_promotions(U64& b, const int& dir) {
  while (b) {
    int to = pop_lsb(b);
    list[last++].m = ((to + dir) | (pop_lsb(b) << 6) | (mt << 12));     // queen
    list[last++].m = ((to + dir) | (pop_lsb(b) << 6) | ((mt+2) << 12)); // rook
    list[last++].m = ((to + dir) | (pop_lsb(b) << 6) | ((mt+4) << 12)); // bishop
    list[last++].m = ((to + dir) | (pop_lsb(b) << 6) | ((mt+6) << 12)); // knight   
  }
}

template<Movetype mt, Piece p, Color c> void movegen<mt, p, c>::print() {
  for(int j=it; j<last; ++j) {    
    std::cout << list[j].to_string() << " ";
  }
  std::cout << "\n";
}


//------------------------------
// white pawn moves
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


template<>
void movegen<capture, pawn, white>::generate(const position& p) {
  
  U64 enemies = p.get_pieces<black>();
  U64 pawns = p.get_pieces<white, pawn>();
  
  // normal captures - non promotions
  U64 left = pawns & pawnmaskleft[white];
  U64 right = pawns & pawnmaskright[white];
  
  if (left != 0ULL) {
    shift<NE>(left);
    left &= enemies;
    if (left != 0ULL) encode_pawn_pushes(left, -9);
  }
  
  if (right != 0ULL) {
    shift<NW>(right);
    right &= enemies;
    if (right != 0ULL) encode_pawn_pushes(right, -7);
  }
}


template<>
void movegen<ep, pawn, white>::generate(const position& p) {

  U64 pawns = p.get_pieces<white, pawn>();
  int eps = p.eps();
  if (eps == Square::no_square) return;

  U64 ep_left = pawns & pawnmaskleft[white] & row[Row::r5]; 
  U64 ep_right = pawns & pawnmaskright[white] & row[Row::r5];
  
  // ep captures
  if (ep_left != 0ULL) {
    shift<NW>(ep_left);
    ep_left &= bitboards::squares[eps]; 
    if (ep_left != 0ULL) encode_pawn_pushes(ep_left, -9);
  }
  
  if (ep_right != 0ULL) {
    shift<NE>(ep_right);
    ep_right &= bitboards::squares[eps];   
    if (ep_right != 0ULL) encode_pawn_pushes(ep_right, -7); 
  }    
}


template<>
void movegen<promotion, pawn, white>::generate(const position& p) {
  U64 pawns = p.get_pieces<white, pawn>();
  U64 pawns7 = pawns & row[Row::r7];
  if (pawns7 == 0ULL) return;
  
  shift<N>(pawns7);
  encode_promotions(pawns7, -8);
}


template<>
void movegen<capture_promotion, pawn, white>::generate(const position& p) {
  U64 pawns = p.get_pieces<white>(); 
  U64 enemies = p.get_pieces<black>() & row[Row::r8];
  U64 right_caps = pawns;
  U64 left_caps = pawns;
  
  shift<NE>(right_caps);
  right_caps &= enemies;
  if (right_caps != 0ULL) encode_promotions(right_caps, -9);
  
  shift<NW>(left_caps);
  left_caps &= enemies;
  if (left_caps != 0ULL) encode_promotions(left_caps, -7);
}

//------------------------------
// black pawn moves
//------------------------------
template<>
void movegen<quiet, pawn, black>::generate(const position& p) {
  
  U64 empty = ~(p.all_pieces());
  U64 pawns = p.get_pieces<black, pawn>();
  U64 single_pushes = pawns & pawnmask[black]; // filter the promotion candidates
  U64 double_pushes = pawns & bitboards::row[r7];

  shift<S>(single_pushes);
  single_pushes &= empty;

  if (single_pushes != 0ULL) encode_pawn_pushes(single_pushes, 8);
  
  for (int i=0; i<2; ++i) {
    shift<S>(double_pushes);
    double_pushes &= empty;
  }

  if (double_pushes != 0ULL) encode_pawn_pushes(double_pushes, 16);
}


template<>
void movegen<capture, pawn, black>::generate(const position& p) {
  
  U64 enemies = p.get_pieces<white>();
  U64 pawns = p.get_pieces<black, pawn>();
  
  // normal captures - non promotions
  U64 left = pawns & pawnmaskleft[black];
  U64 right = pawns & pawnmaskright[black];
  
  if (left != 0ULL) {
    shift<SE>(left);
    left &= enemies;
    if (left != 0ULL) encode_pawn_pushes(left, 7);
  }
  
  if (right != 0ULL) {
    shift<SW>(right);
    right &= enemies;
    if (right != 0ULL) encode_pawn_pushes(right, 9);
  }
}


template<>
void movegen<ep, pawn, black>::generate(const position& p) {

  U64 pawns = p.get_pieces<black, pawn>();
  int eps = p.eps();
  if (eps == Square::no_square) return;

  U64 ep_left = pawns & pawnmaskleft[black] & row[Row::r4]; 
  U64 ep_right = pawns & pawnmaskright[black] & row[Row::r4];
  
  // ep captures
  if (ep_left != 0ULL) {
    shift<SW>(ep_left);
    ep_left &= bitboards::squares[eps]; 
    if (ep_left != 0ULL) encode_pawn_pushes(ep_left, 9);
  }
  
  if (ep_right != 0ULL) {
    shift<SE>(ep_right);
    ep_right &= bitboards::squares[eps];   
    if (ep_right != 0ULL) encode_pawn_pushes(ep_right, 7); 
  }    
}


template<>
void movegen<promotion, pawn, black>::generate(const position& p) {
  U64 pawns = p.get_pieces<black, pawn>();
  U64 pawns7 = pawns & row[Row::r2];
  if (pawns7 == 0ULL) return;
  
  shift<S>(pawns7);
  encode_promotions(pawns7, 8);
}


template<>
void movegen<capture_promotion, pawn, black>::generate(const position& p) {
  U64 pawns = p.get_pieces<black>(); 
  U64 enemies = p.get_pieces<white>() & row[Row::r1];
  U64 right_caps = pawns;
  U64 left_caps = pawns;
  
  shift<SE>(right_caps);
  right_caps &= enemies;
  if (right_caps != 0ULL) encode_promotions(right_caps, 7);
  
  shift<SW>(left_caps);
  left_caps &= enemies;
  if (left_caps != 0ULL) encode_promotions(left_caps, 9);
}

//------------------------------
// knight moves
//------------------------------
template<>
void movegen<quiet, knight, white>::generate(const position& p) {  
  U64 empty = ~p.all_pieces();  
  for (auto& s : p.squares_of<white, knight>()) {
    if (s == Square::no_square) break;    
    U64 mvs = bitboards::nmask[s] & empty;
    if (mvs != 0ULL)  { encode(mvs, s); }    
  }
}

template<>
void movegen<capture, knight, white>::generate(const position& p) {  
  U64 enemies = p.get_pieces<black>();  
  for (auto& s : p.squares_of<white, knight>()) {
    if (s == Square::no_square) break;    
    U64 mvs = bitboards::nmask[s] & enemies;
    if (mvs != 0ULL)  { encode(mvs, s); }    
  }
}

template<>
void movegen<quiet, knight, black>::generate(const position& p) {  
  U64 empty = ~p.all_pieces();  
  for (auto& s : p.squares_of<black, knight>()) {
    if (s == Square::no_square) break;    
    U64 mvs = bitboards::nmask[s] & empty;
    if (mvs != 0ULL)  { encode(mvs, s); }    
  }
}

template<>
void movegen<capture, knight, black>::generate(const position& p) {  
  U64 enemies = p.get_pieces<white>();  
  for (auto& s : p.squares_of<black, knight>()) {
    if (s == Square::no_square) break;    
    U64 mvs = bitboards::nmask[s] & enemies;
    if (mvs != 0ULL)  { encode(mvs, s); }    
  }
}

//------------------------------
// bishop moves
//------------------------------
template<>
void movegen<quiet, bishop, white>::generate(const position& p) {  
  U64 mask = p.all_pieces();
  U64 empty = ~mask;
  for (auto& s : p.squares_of<white, bishop>()) {
    if (s == Square::no_square) break;
    U64 mvs = magics::attacks<bishop>(mask, s) & empty;
    if (mvs != 0ULL)  { encode(mvs, s); }    
  }
}

template<>
void movegen<capture, bishop, white>::generate(const position& p) {  
  U64 mask = p.all_pieces();
  U64 enemies = p.get_pieces<black>();
  for (auto& s : p.squares_of<white, bishop>()) {
    if (s == Square::no_square) break;
    U64 mvs = magics::attacks<bishop>(mask, s) & enemies;
    if (mvs != 0ULL)  { encode(mvs, s); }    
  }
}

template<>
void movegen<quiet, bishop, black>::generate(const position& p) {  
  U64 mask = p.all_pieces();
  U64 empty = ~mask;
  for (auto& s : p.squares_of<black, bishop>()) {
    if (s == Square::no_square) break;
    U64 mvs = magics::attacks<bishop>(mask, s) & empty;
    if (mvs != 0ULL)  { encode(mvs, s); }    
  }
}

template<>
void movegen<capture, bishop, black>::generate(const position& p) {  
  U64 mask = p.all_pieces();
  U64 enemies = p.get_pieces<white>();
  for (auto& s : p.squares_of<black, bishop>()) {
    if (s == Square::no_square) break;
    U64 mvs = magics::attacks<bishop>(mask, s) & enemies;
    if (mvs != 0ULL)  { encode(mvs, s); }    
  }
}

//------------------------------
// rook moves
//------------------------------
template<>
void movegen<quiet, rook, white>::generate(const position& p) {  
  U64 mask = p.all_pieces();
  U64 empty = ~mask;
  for (auto& s : p.squares_of<white, rook>()) {
    if (s == Square::no_square) break;
    U64 mvs = magics::attacks<rook>(mask, s) & empty;
    if (mvs != 0ULL) { encode(mvs, s); }    
  }
}

template<>
void movegen<capture, rook, white>::generate(const position& p) {  
  U64 mask = p.all_pieces();
  U64 enemies = p.get_pieces<black>();
  for (auto& s : p.squares_of<white, rook>()) {
    if (s == Square::no_square) break;
    U64 mvs = magics::attacks<rook>(mask, s) & enemies;
    if (mvs != 0ULL) { encode(mvs, s); }    
  }
}

template<>
void movegen<quiet, rook, black>::generate(const position& p) {  
  U64 mask = p.all_pieces();
  U64 empty = ~mask;
  for (auto& s : p.squares_of<black, rook>()) {
    if (s == Square::no_square) break;
    U64 mvs = magics::attacks<rook>(mask, s) & empty;
    if (mvs != 0ULL) { encode(mvs, s); }    
  }
}

template<>
void movegen<capture, rook, black>::generate(const position& p) {  
  U64 mask = p.all_pieces();
  U64 enemies = p.get_pieces<white>();
  for (auto& s : p.squares_of<black, rook>()) {
    if (s == Square::no_square) break;
    U64 mvs = magics::attacks<rook>(mask, s) & enemies;
    if (mvs != 0ULL) { encode(mvs, s); }    
  }
}
