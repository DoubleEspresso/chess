#include "move2.h"
#include "board.h"
#include "magic.h"
#include "uci.h"

using namespace Globals;

// move generation utilities
namespace {  
  template<Direction> inline void shift(U64& b);
  template<> inline void shift<NORTH>(U64& b)  { b <<= 8; }
  template<> inline void shift<SOUTH>(U64& b)  { b >>= 8; }
  template<> inline void shift<NN>(U64& b) { b <<= 16; }
  template<> inline void shift<SS>(U64& b) { b >>= 16; }
  template<> inline void shift<NW>(U64& b) { b <<= 7; }
  template<> inline void shift<NE>(U64& b) { b <<= 9; }
  template<> inline void shift<SW>(U64& b) { b >>= 7; }
  template<> inline void shift<SE>(U64& b) { b >>= 9; }  
}

template<MoveType2 mt, Piece p, Color c>
template<Direction d> void MoveGenerator2<mt, p, c>::serialize(U64& b) {
  while (b) {
    int to = pop_lsb(b);
    list[last++].m = ((to + d) | (to << 6) | (mt << 12));
  }
}

template<MoveType2 mt, Piece p, Color c> void MoveGenerator2<mt, p, c>::print() {
  for(int j=it; j<last; ++j) {
    U16 move = list[j].m;
    printf("%s ", UCI::move_to_string(move).c_str());
  }
  printf("\n");
}

template<> 
void MoveGenerator2<QUIETS, PAWN, WHITE>::generate(Board& b) {

  U64 empty = ~(b.all_pieces());
  U64 pawns = b.get_pieces(WHITE, PAWN);
  U64 rank2 = RowBB[ROW2];
  U64 rank7 = RowBB[ROW7];
  U64 pawns7 = pawns & rank7;
  U64 single_pushes = pawns ^ pawns7;
  U64 double_pushes = pawns & rank2;

  shift<NORTH>(single_pushes);
  single_pushes &= empty;

  if (single_pushes != 0ULL) serialize<SOUTH>(single_pushes);
  
  for (int i=0; i<2; ++i) {
    shift<NORTH>(double_pushes);
    double_pushes &= empty;
  }

  if (double_pushes != 0ULL) serialize<SS>(double_pushes);
}

template<>
void MoveGenerator2<CAPTURES, PAWN, WHITE>::generate(Board& b) {

  U64 enemies = b.colored_pieces(BLACK);
  U64 pawns = b.colored_pieces(WHITE);
  U64 rank7 = RowBB[ROW7];
  U64 pawns7 = pawns & rank7;
  U64 pawnsNot7 = pawns ^ pawns7;

  // normal captures - non promotions
  U64 left = pawnsNot7 ^ (pawnsNot7 & ColBB[COL8]);
  U64 right = pawnsNot7 ^ (pawnsNot7 & ColBB[COL1]);  

  if (left != 0ULL) {
    shift<NE>(left);
    left &= enemies;
    if (left != 0ULL) serialize<SW>(left);
  }
  
  if (right != 0ULL) {
    shift<NW>(right);
    right &= enemies;
    if (right != 0ULL) serialize<SE>(right);
  }
}

template<>
void MoveGenerator2<EP2, PAWN, WHITE>::generate(Board& b) {

  U64 enemies = b.colored_pieces(BLACK);
  U64 pawns = b.colored_pieces(WHITE);
  int eps = b.get_ep_sq();

  if (eps == SQUARE_NONE) return;

  U64 ep_left = (pawns ^ (pawns & ColBB[COL8])) & RowBB[ROW5];
  U64 ep_right = (pawns ^ (pawns & ColBB[COL1])) & RowBB[ROW5];
  
  // ep captures
  if (ep_left != 0ULL) {
    shift<NW>(ep_left);
    ep_left &= SquareBB[eps];
    if (ep_left != 0ULL) serialize<SE>(ep_left);
  }

  if (ep_right != 0ULL) {
    shift<NE>(ep_right);
    ep_right &= SquareBB[eps];   
    if (ep_right != 0ULL) serialize<SW>(ep_right);
  }    
}  


template<>
void MoveGenerator2<QUIETS, PAWN, BLACK>::generate(Board& b) {
  
  U64 empty = ~(b.all_pieces());
  U64 pawns = b.get_pieces(BLACK, PAWN);
  U64 rank2 = RowBB[ROW7];
  U64 rank7 = RowBB[ROW2];
  U64 pawns7 = pawns & rank7;
  U64 single_pushes = pawns ^ pawns7;
  U64 double_pushes = pawns & rank2;

  shift<SOUTH>(single_pushes);
  single_pushes &= empty;

  if (single_pushes != 0ULL) serialize<NORTH>(single_pushes);
  
  for (int i=0; i<2; ++i) {
    shift<SOUTH>(double_pushes);
    double_pushes &= empty;
  }

  if (double_pushes != 0ULL) serialize<NN>(double_pushes);
}
