#include "move2.h"
#include "board.h"
#include "magic.h"
#include "uci.h"
#include "bits.h"

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

  //template<MoveType2, Piece, Color> inline void generate(Board& b);
}

template<MoveType2 mt, Piece p, Color c>
template<Direction d> void MoveGenerator2<mt, p, c>::serialize(U64& b) {
  while (b) {
    int to = pop_lsb(b);
    list[last++].m = ((to + d) | (to << 6) | (mt << 12));
  }
}

template<MoveType2 mt, Piece p, Color c>
void MoveGenerator2<mt, p, c>::serialize(U64& b, const int& f) {
  while (b) list[last++].m = (f | (pop_lsb(b) << 6) | (mt << 12));
}

template<MoveType2 mt, Piece p, Color c>
template<Direction d> void MoveGenerator2<mt, p, c>::serialize_promotions(U64& b) {
  while (b) {
    int to = pop_lsb(b);
    for (int i = mt - 3; i <= mt; ++i) {
      list[last++].m = ((to + d) | (to << 6) | (i << 12));
    }    
  }
}

template<MoveType2 mt, Piece p, Color c> void MoveGenerator2<mt, p, c>::print() {
  for(int j=it; j<last; ++j) {
    U16 move = list[j].m;
    printf("%s ", UCI::move_to_string(move).c_str());
  }
  printf("\n");
}

//-----------------------------------------------------------------------------
// start white pawn moves
//-----------------------------------------------------------------------------

template<>
void MoveGenerator2<QUIETS, PAWN, WHITE>::generate(Board& b) { 

  U64 empty = ~(b.all_pieces());
  U64 pawns = b.get_pieces(WHITE, PAWN);
  U64 single_pushes = pawns & PawnMaskAll[WHITE];
  U64 double_pushes = pawns & RowBB[ROW2];

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

  // normal captures - non promotions
  U64 left = pawns & PawnMaskLeft[WHITE];
  U64 right = pawns & PawnMaskRight[WHITE];

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

  U64 pawns = b.colored_pieces(WHITE);
  U64 row5 = RowBB[ROW5];
  int eps = b.get_ep_sq();
  if (eps == SQUARE_NONE) return;

  U64 ep_left = pawns & PawnMaskLeft[WHITE] & row5; 
  U64 ep_right = pawns & PawnMaskRight[WHITE] & row5;
  
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
void MoveGenerator2<PROMOTIONS, PAWN, WHITE>::generate(Board& b) {
  U64 pawns = b.colored_pieces(WHITE);
  U64 pawns7 = pawns & RowBB[ROW7];
  if (pawns7 == 0ULL) return;
  
  shift<NORTH>(pawns7);
  if (pawns7) serialize_promotions<SOUTH>(pawns7);
}

template<>
void MoveGenerator2<CAPTURE_PROMOTIONS, PAWN, WHITE>::generate(Board& b) {
  U64 pawns = b.colored_pieces(WHITE);
  U64 enemies = b.colored_pieces(BLACK) & RowBB[ROW8];  
  U64 right_caps = pawns;
  U64 left_caps = pawns;
  
  shift<NE>(right_caps);
  right_caps &= enemies;
  if (right_caps != 0ULL) serialize_promotions<SW>(right_caps);
  
  shift<NW>(left_caps);
  left_caps &= enemies;
  if (left_caps != 0ULL) serialize_promotions<SE>(left_caps);
}
//-----------------------------------------------------------------------------
// end white pawn moves
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// start black pawn moves
//-----------------------------------------------------------------------------
template<>
void MoveGenerator2<QUIETS, PAWN, BLACK>::generate(Board& b) {
  
  U64 empty = ~(b.all_pieces());
  U64 pawns = b.get_pieces(BLACK, PAWN);
  U64 single_pushes = pawns & PawnMaskAll[BLACK];
  U64 double_pushes = pawns & RowBB[ROW7];

  shift<SOUTH>(single_pushes);
  single_pushes &= empty; 
  if (single_pushes != 0ULL) serialize<NORTH>(single_pushes);
  
  for (int i=0; i<2; ++i) {
    shift<SOUTH>(double_pushes);
    double_pushes &= empty;
  }
  if (double_pushes != 0ULL) serialize<NN>(double_pushes);
}

template<>
void MoveGenerator2<CAPTURES, PAWN, BLACK>::generate(Board& b) {

  U64 enemies = b.colored_pieces(WHITE);
  U64 pawns = b.colored_pieces(BLACK);

  // normal captures - non promotions
  U64 left = pawns & PawnMaskLeft[BLACK];
  U64 right = pawns & PawnMaskRight[BLACK];

  if (left != 0ULL) {
    shift<SE>(left);
    left &= enemies;
    if (left != 0ULL) serialize<NW>(left);
  }
  
  if (right != 0ULL) {
    shift<SW>(right);
    right &= enemies;
    if (right != 0ULL) serialize<NE>(right);
  }
}

template<>
void MoveGenerator2<EP2, PAWN, BLACK>::generate(Board& b) {

  U64 pawns = b.colored_pieces(BLACK);
  U64 row4 = RowBB[ROW4];
  int eps = b.get_ep_sq();
  if (eps == SQUARE_NONE) return;

  U64 ep_left = pawns & PawnMaskLeft[BLACK] & row4; 
  U64 ep_right = pawns & PawnMaskRight[BLACK] & row4;
  
  // ep captures
  if (ep_left != 0ULL) {
    shift<SW>(ep_left);
    ep_left &= SquareBB[eps];
    if (ep_left != 0ULL) serialize<NE>(ep_left);
  }

  if (ep_right != 0ULL) {
    shift<SE>(ep_right);
    ep_right &= SquareBB[eps];   
    if (ep_right != 0ULL) serialize<NW>(ep_right);
  }    
}  

template<>
void MoveGenerator2<PROMOTIONS, PAWN, BLACK>::generate(Board& b) {
  U64 pawns = b.colored_pieces(BLACK);
  U64 pawns2 = pawns & RowBB[ROW2];
  if (pawns2 == 0ULL) return;
  
  shift<SOUTH>(pawns2);
  if (pawns2) serialize_promotions<NORTH>(pawns2);
}

template<>
void MoveGenerator2<CAPTURE_PROMOTIONS, PAWN, BLACK>::generate(Board& b) {
  U64 pawns = b.colored_pieces(BLACK);
  U64 enemies = b.colored_pieces(WHITE) & RowBB[ROW1];  
  U64 right_caps = pawns;
  U64 left_caps = pawns;
  
  shift<NE>(right_caps);
  right_caps &= enemies;
  if (right_caps != 0ULL) serialize_promotions<SW>(right_caps);
  
  shift<NW>(left_caps);
  left_caps &= enemies;
  if (left_caps != 0ULL) serialize_promotions<SE>(left_caps);
}
//-----------------------------------------------------------------------------
// end black pawn moves
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// start quiet piece moves
//-----------------------------------------------------------------------------
template<>
void MoveGenerator2<QUIETS, KNIGHT, WHITE>::generate(Board& b) {
  U64  empty = ~b.all_pieces();
  int *sqs = b.sq_of<KNIGHT>(WHITE);
  for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs) {
    U64 quites = PseudoAttacksBB(KNIGHT, from) & empty;
    if (quites) serialize(quites, from);
  }
}

template<>
void MoveGenerator2<QUIETS, KNIGHT, BLACK>::generate(Board& b) {
  U64  empty = ~b.all_pieces();
  int *sqs = b.sq_of<KNIGHT>(BLACK);
  for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs) {
    U64 quites = PseudoAttacksBB(KNIGHT, from) & empty;
    if (quites != 0ULL) serialize(quites, from);
  }
}

template<>
void MoveGenerator2<QUIETS, KING, WHITE>::generate(Board& b) {
  int f = b.king_square(WHITE);
  U64 mvs = PseudoAttacksBB(KING, f) & (~b.all_pieces());
  if (mvs != 0ULL) serialize(mvs, f); 
}

template<>
void MoveGenerator2<QUIETS, KING, BLACK>::generate(Board& b) {
  int f = b.king_square(BLACK);
  U64 mvs = PseudoAttacksBB(KING, f) & (~b.all_pieces());
  if (mvs != 0ULL) serialize(mvs, f); 
}

template<>
void MoveGenerator2<CAPTURES, KING, WHITE>::generate(Board& b) {
  int f = b.king_square(WHITE);
  U64 mvs = PseudoAttacksBB(KING, f) & (b.colored_pieces(BLACK));
  if (mvs != 0ULL) serialize(mvs, f); 
}

template<>
void MoveGenerator2<CAPTURES, KING, BLACK>::generate(Board& b) {
  int f = b.king_square(BLACK);
  U64 mvs = PseudoAttacksBB(KING, f) & (b.colored_pieces(WHITE));
  if (mvs != 0ULL) serialize(mvs, f); 
}

template<>
void MoveGenerator2<QUIETS, BISHOP, WHITE>::generate(Board& b) {
  int * sqs = b.sq_of<BISHOP>(WHITE);
  U64 mask = b.all_pieces();
  U64 empty = ~mask;
  for (int f = *++sqs; f != SQUARE_NONE; f = *++sqs) {
    U64 mvs = Magic::attacks<BISHOP>(mask, f);
    mvs &= empty;
    if (mvs != 0ULL) serialize(mvs, f);
  }
}

template<>
void MoveGenerator2<QUIETS, BISHOP, BLACK>::generate(Board& b) {
  int * sqs = b.sq_of<BISHOP>(BLACK);
  U64 mask = b.all_pieces();
  U64 empty = ~mask;
  for (int f = *++sqs; f != SQUARE_NONE; f = *++sqs) {
    U64 mvs = Magic::attacks<BISHOP>(mask, f);
    mvs &= empty;
    if (mvs != 0ULL) serialize(mvs, f);
  }
}

template<>
void MoveGenerator2<CAPTURES, BISHOP, WHITE>::generate(Board& b) {
  int * sqs = b.sq_of<BISHOP>(WHITE);
  U64 mask = b.all_pieces();
  U64 enemies = b.colored_pieces(BLACK);
  for (int f = *++sqs; f != SQUARE_NONE; f = *++sqs) {
    U64 mvs = Magic::attacks<BISHOP>(mask, f);
    mvs &= enemies;
    if (mvs != 0ULL) serialize(mvs, f);
  }
}

template<>
void MoveGenerator2<CAPTURES, BISHOP, BLACK>::generate(Board& b) {
  int * sqs = b.sq_of<BISHOP>(BLACK);
  U64 mask = b.all_pieces();
  U64 enemies = b.colored_pieces(WHITE);
  for (int f = *++sqs; f != SQUARE_NONE; f = *++sqs) {
    U64 mvs = Magic::attacks<BISHOP>(mask, f);
    mvs &= enemies;
    if (mvs != 0ULL) serialize(mvs, f);
  }
}

template<>
void MoveGenerator2<QUIETS, ROOK, WHITE>::generate(Board& b) {
  int * sqs = b.sq_of<ROOK>(WHITE);
  U64 mask = b.all_pieces();
  U64 empty = ~mask;
  for (int f = *++sqs; f != SQUARE_NONE; f = *++sqs) {
    U64 mvs = Magic::attacks<ROOK>(mask, f);
    mvs &= empty;
    if (mvs != 0ULL) serialize(mvs, f);
  }
}

template<>
void MoveGenerator2<QUIETS, ROOK, BLACK>::generate(Board& b) {
  int * sqs = b.sq_of<ROOK>(BLACK);
  U64 mask = b.all_pieces();
  U64 empty = ~mask;
  for (int f = *++sqs; f != SQUARE_NONE; f = *++sqs) {
    U64 mvs = Magic::attacks<ROOK>(mask, f);
    mvs &= empty;
    if (mvs != 0ULL) serialize(mvs, f);
  }
}

template<>
void MoveGenerator2<CAPTURES, ROOK, WHITE>::generate(Board& b) {
  int * sqs = b.sq_of<ROOK>(WHITE);
  U64 mask = b.all_pieces();
  U64 enemies = b.colored_pieces(BLACK);
  for (int f = *++sqs; f != SQUARE_NONE; f = *++sqs) {
    U64 mvs = Magic::attacks<ROOK>(mask, f);
    mvs &= enemies;
    if (mvs != 0ULL) serialize(mvs, f);
  }
}

template<>
void MoveGenerator2<CAPTURES, ROOK, BLACK>::generate(Board& b) {
  int * sqs = b.sq_of<ROOK>(BLACK);
  U64 mask = b.all_pieces();
  U64 enemies = b.colored_pieces(WHITE);
  for (int f = *++sqs; f != SQUARE_NONE; f = *++sqs) {
    U64 mvs = Magic::attacks<ROOK>(mask, f);
    mvs &= enemies;
    if (mvs != 0ULL) serialize(mvs, f);
  }
}

template<>
void MoveGenerator2<QUIETS, QUEEN, WHITE>::generate(Board& b) {
  int * sqs = b.sq_of<QUEEN>(WHITE);
  U64 mask = b.all_pieces();
  U64 empty = ~mask;
  for (int f = *++sqs; f != SQUARE_NONE; f = *++sqs) {
    U64 mvs = (Magic::attacks<ROOK>(mask, f) | Magic::attacks<BISHOP>(mask, f));
    mvs &= empty;
    if (mvs != 0ULL) serialize(mvs, f);
  }
}

template<>
void MoveGenerator2<QUIETS, QUEEN, BLACK>::generate(Board& b) {
  int * sqs = b.sq_of<QUEEN>(BLACK);
  U64 mask = b.all_pieces();
  U64 empty = ~mask;
  for (int f = *++sqs; f != SQUARE_NONE; f = *++sqs) {
    U64 mvs = (Magic::attacks<ROOK>(mask, f) | Magic::attacks<BISHOP>(mask, f));
    mvs &= empty;
    if (mvs != 0ULL) serialize(mvs, f);
  }
}

template<>
void MoveGenerator2<CAPTURES, QUEEN, WHITE>::generate(Board& b) {
  int * sqs = b.sq_of<QUEEN>(WHITE);
  U64 mask = b.all_pieces();
  U64 enemies = b.colored_pieces(BLACK);
  for (int f = *++sqs; f != SQUARE_NONE; f = *++sqs) {
    U64 mvs = (Magic::attacks<ROOK>(mask, f) | Magic::attacks<BISHOP>(mask, f));
    mvs &= enemies;
    if (mvs != 0ULL) serialize(mvs, f);
  }
}

template<>
void MoveGenerator2<CAPTURES, QUEEN, BLACK>::generate(Board& b) {
  int * sqs = b.sq_of<QUEEN>(BLACK);
  U64 mask = b.all_pieces();
  U64 enemies = b.colored_pieces(WHITE);
  for (int f = *++sqs; f != SQUARE_NONE; f = *++sqs) {
    U64 mvs = (Magic::attacks<ROOK>(mask, f) | Magic::attacks<BISHOP>(mask, f));
    mvs &= enemies;
    if (mvs != 0ULL) serialize(mvs, f);
  }
}
