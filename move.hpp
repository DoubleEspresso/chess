#include "move.h"
#include "magics.h"
#include "bits.h"
#include "bitboards.h"
#include "position.h"


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


template<Movetype mt>
inline void Movegen::encode(U64& b, const Square& f) {
  while (b) list[last++].set(f, pop_lsb(b), mt);
}

template<Movetype mt>
inline void Movegen::encode_pawn_pushes(U64& b, const int& dir) {
  while (b) {
    U8 to = pop_lsb(b);
    list[last++].set(to + dir, to, mt);
  }
}

inline void Movegen::encode_capture_promotions(U64& b, const int& dir) {
  while (b) {
    U8 to = pop_lsb(b);
    U8 frm = to + dir;
    list[last++].set(frm, to, capture_promotion_q);
    list[last++].set(frm, to, capture_promotion_r);
    list[last++].set(frm, to, capture_promotion_b); 
    list[last++].set(frm, to, capture_promotion_n); 
  }
}

inline void Movegen::encode_quiet_promotions(U64& b, const int& dir) {
  while (b) {
    U8 to = pop_lsb(b);
    U8 frm = to + dir;
    list[last++].set(frm, to, promotion_q);
    list[last++].set(frm, to, promotion_r);
    list[last++].set(frm, to, promotion_b); 
    list[last++].set(frm, to, promotion_n); 
  }
}

inline void Movegen::print() {
  for(int j=it; j<last; ++j) {    
    std::cout << SanSquares[list[j].f] << SanSquares[list[j].t] << " "; 
  }
  std::cout << "\n";
}

inline void Movegen::print_legal(position& p) {
  for(int j=it; j<last; ++j) {    
    if (p.is_legal(list[j])) std::cout << SanSquares[list[j].f] << SanSquares[list[j].t] << " ";
  }
  std::cout << "\n";
}


//----------------------------------------------
// movegen utilities
//----------------------------------------------

inline void Movegen::initialize(const position& p) {
  us = p.to_move();
  them = Color(us ^ 1);
  all_pieces = p.all_pieces();
  empty = ~all_pieces;

  can_castle_ks = p.can_castle_ks();
  can_castle_qs = p.can_castle_qs();

  check_target = p.checkers(); // checking piece(s)
  evasion_target = 0ULL;
  
  
  if (check_target != 0ULL && !bits::more_than_one(check_target)) { // only 1 checker
    U64 tmp = check_target;
    Square frm = Square(pop_lsb(tmp));
    Piece checker = p.piece_on(frm);
    if (checker == bishop || checker == rook || checker == queen) {      
      evasion_target = bitboards::between[frm][p.king_square()] & empty;
    }
  }
  
  if (us == white) {
    tmp_clock.start();
    rank2 = bitboards::row[r2];
    rank7 = bitboards::row[r7];
    pawns = p.get_pieces<white, pawn>();
    knights = p.squares_of<white, knight>();    
    bishops = p.squares_of<white, bishop>();
    rooks = p.squares_of<white, rook>();
    queens = p.squares_of<white, queen>();
    kings = p.squares_of<white, king>();
    enemies = p.get_pieces<black>();
  }
  else {
    tmp_clock.start();
    rank2 = bitboards::row[r7];
    rank7 = bitboards::row[r2];
    pawns = p.get_pieces<black, pawn>();
    knights = p.squares_of<black, knight>();
    bishops = p.squares_of<black, bishop>();
    rooks = p.squares_of<black, rook>();
    queens = p.squares_of<black, queen>();
    kings = p.squares_of<black, king>();
    enemies = p.get_pieces<white>();
  }

  qtarget = (evasion_target != 0ULL ? evasion_target : empty);
  ctarget = (check_target == 0ULL ? enemies : check_target);

  eps = p.eps();
  pawns2 = pawns & rank2;
  pawns7 = pawns & rank7;

}


inline void Movegen::pawn_caps(U64& left, U64& right, U64& ep_left, U64& ep_right) {  
  // normal captures - non promotions
  left = pawns & pawnmaskleft[us];
  right = pawns & pawnmaskright[us];   
  
  auto sw = (us == white ? shift<NW> : shift<SE>);
  auto se = (us == white ? shift<NE> : shift<SW>);
  
  if (left != 0ULL) {
    se(left);
    left &= ctarget;
  }
  
  if (right != 0ULL) {
    sw(right);
    right &= ctarget;
  }

  // ep captures - evasions are checked individually
  if (eps == Square::no_square) return;
  auto r = (us == white ? Row::r5 : Row::r4);
  ep_left = pawns & pawnmaskleft[us] & row[r]; 
  ep_right = pawns & pawnmaskright[us] & row[r];
  
  if (ep_left != 0ULL) {
    se(ep_left);
    ep_left &= bitboards::squares[eps];
  }
  
  if (ep_right != 0ULL) {
    sw(ep_right);
    ep_right &= bitboards::squares[eps];
  }    
}

//------------------------------
// pawn moves
//------------------------------

template<>
inline void Movegen::generate<quiet, pawn>() {
  
  U64 single = pawns & pawnmask[us]; // filter the promotion candidates
  U64 dbl = pawns & rank2;
  
  int d1 = (us == white ? -8 : 8);
  int d2 = 2 * d1;
  
  auto s = (us == white ? shift<N> : shift<S>);

  // single pawn pushes
  if (single) {
    s(single);
    single &= qtarget;
  }

  // double pawn pushes
  if (dbl) {
    s(dbl);
    dbl &= empty;

    if (dbl) {
      s(dbl);
      dbl &= empty;
      if (evasion_target) dbl &= evasion_target;
    }
  }

  if (single != 0ULL) encode_pawn_pushes<quiet>(single, d1);  
  if (dbl != 0ULL) encode_pawn_pushes<quiet>(dbl, d2);
}


template<>
inline void Movegen::generate<capture, pawn>() {
  U64 left = 0ULL;
  U64 right = 0ULL;
  U64 ep_left = 0ULL;
  U64 ep_right = 0ULL;
  
  int d1 = (us == white ? -7 : 9);
  int d2 = (us == white ? -9 : 7);

  pawn_caps(left, right, ep_left, ep_right);
  
  if (left != 0ULL) encode_pawn_pushes<capture>(left, d2);
  if (right != 0ULL) encode_pawn_pushes<capture>(right, d1);
  
  if (ep_left != 0ULL) encode_pawn_pushes<ep>(ep_left, d2);  
  if (ep_right != 0ULL) encode_pawn_pushes<ep>(ep_right, d1);  
}

template<>
inline void Movegen::generate<promotion, pawn>() {
  if (pawns7 == 0ULL) return;
  
  U64 quiets = pawns7;

  auto s = (us == white ? shift<N> : shift<S>);
  
  s(quiets);

  quiets &= qtarget;
  
  if (quiets) encode_quiet_promotions(quiets, us == white ? -8 : 8);
}

template<>
inline void Movegen::generate<capture_promotion, pawn>() {
  
  if (pawns7 == 0ULL) return;

  Color them = Color(us ^ 1);
  U64 caps_l = pawns7 & pawnmaskleft[them];
  U64 caps_r = pawns7 & pawnmaskright[them];
  int d1 = (us == white ? -7 : 9);
  int d2 = (us == white ? -9 : 7);
  
  auto sw = (us == white ? shift<NW> : shift<SE>);
  auto se = (us == white ? shift<NE> : shift<SW>);

  if (caps_l) {
    se(caps_l);
    caps_l &= ctarget;
  }

  if (caps_r) {
    sw(caps_r);  
    caps_r &= ctarget;
  }
  
  if (caps_r != 0ULL) encode_capture_promotions(caps_r, d1);
  if (caps_l != 0ULL) encode_capture_promotions(caps_l, d2);
}


//------------------------------
// knight moves
//------------------------------
template<>
inline void Movegen::generate<quiet, knight>() {
  for (Square s = *knights; s != no_square; s = *++knights) {
    U64 mvs = bitboards::nmask[s] & qtarget;    
    if (mvs != 0ULL) encode<quiet>(mvs, s);
  }
}

template<>
inline void Movegen::generate<capture, knight>() {
  for (Square s = *knights; s != no_square; s = *++knights) {
    U64 mvs = bitboards::nmask[s] & ctarget;     
    if ( mvs != 0ULL) encode<capture>(mvs, s);
  }
}

template<>
inline void Movegen::generate<pseudo_legal, knight>() {
  for (Square s = *knights; s != no_square; s = *++knights) {
    U64 qmvs = bitboards::nmask[s] & qtarget;     
    if (qmvs != 0ULL) encode<quiet>(qmvs, s);
    
    U64 cmvs = bitboards::nmask[s] & ctarget;     
    if (cmvs != 0ULL) encode<capture>(cmvs, s);
  }
}

//------------------------------
// bishop moves
//------------------------------
template<>
inline void Movegen::generate<quiet, bishop>() {  
  for (Square s = *bishops; s != no_square; s = *++bishops) {
    U64 mvs = magics::attacks<bishop>(all_pieces, s) & qtarget;
    if (mvs != 0ULL) encode<quiet>(mvs, s);
  }  
}

template<>
inline void Movegen::generate<capture, bishop>() {
  for (Square s = *bishops; s != no_square; s = *++bishops) {
    U64 mvs = magics::attacks<bishop>(all_pieces, s) & ctarget;
    if (mvs != 0ULL) encode<capture>(mvs, s);
  }
}

template<>
inline void Movegen::generate<pseudo_legal, bishop>() {
  for (Square s = *bishops; s != no_square; s = *++bishops) {
    U64 mvs = magics::attacks<bishop>(all_pieces, s);

    U64 q = mvs & qtarget;
    if (q != 0ULL) encode<quiet>(q, s);
    
    U64 c = mvs & ctarget;
    if (c != 0ULL) encode<capture>(c, s);
  }
}

//------------------------------
// rook moves
//------------------------------
template<>
inline void Movegen::generate<quiet, rook>() {
  for (Square s = *rooks; s != no_square; s = *++rooks) {
    U64 mvs = magics::attacks<rook>(all_pieces, s) & qtarget;
    if (mvs != 0ULL) encode<quiet>(mvs, s);
  }   
}

template<>
inline void Movegen::generate<capture, rook>() {
  for (Square s = *rooks; s != no_square; s = *++rooks) {
    U64 mvs = magics::attacks<rook>(all_pieces, s) & ctarget;
    if (mvs != 0ULL) encode<capture>(mvs, s);
  }
}

template<>
inline void Movegen::generate<pseudo_legal, rook>() {
  for (Square s = *rooks; s != no_square; s = *++rooks) {
    U64 mvs = magics::attacks<rook>(all_pieces, s);

    U64 q = mvs & qtarget;
    if (q != 0ULL) encode<quiet>(q, s);

    U64 c = mvs & ctarget;
    if (c != 0ULL) encode<capture>(c, s);
  }
}

//------------------------------
// queen moves
//------------------------------
template<>
inline void Movegen::generate<quiet, queen>() {
  for (Square s = *queens; s != no_square; s = *++queens) {
    U64 mvs = (magics::attacks<bishop>(all_pieces, s) |
	       magics::attacks<rook>(all_pieces, s)) & qtarget;
    if (mvs != 0ULL) encode<quiet>(mvs, s);
  }   
}

template<>
inline void Movegen::generate<capture, queen>() {  
  for (Square s = *queens; s != no_square; s = *++queens) {
    
    U64 mvs = (magics::attacks<bishop>(all_pieces, s) |
	       magics::attacks<rook>(all_pieces, s)) & ctarget;
    if (mvs != 0ULL) encode<capture>(mvs, s);
  }
}

template<>
inline void Movegen::generate<pseudo_legal, queen>() {
  for (Square s = *queens; s != no_square; s = *++queens) {
    
    U64 mvs = (magics::attacks<bishop>(all_pieces, s) |
	       magics::attacks<rook>(all_pieces, s));

    U64 q = mvs & qtarget;
    if (q != 0ULL) encode<quiet>(q, s);

    U64 c = mvs & ctarget;
    if (c != 0ULL) encode<capture>(c, s);
  }
}

//------------------------------
// king moves
//------------------------------
template<>
inline void Movegen::generate<quiet, king>() {
  U64 mvs = (bitboards::kmask[kings[0]] & empty);
  if (mvs != 0ULL) encode<quiet>(mvs, kings[0]);
}

template<>
inline void Movegen::generate<castles, king>() {
  const Square f = (us == white ? E1 : E8);
  if (can_castle_ks) list[last++].set(f, (us == white ? G1 : G8), castle_ks);
  if (can_castle_qs) list[last++].set(f, (us == white ? C1 : C8), castle_qs);
}

template<>
inline void Movegen::generate<capture, king>() {
  U64 mvs = (bitboards::kmask[kings[0]] & enemies);
  if (mvs != 0ULL) encode<capture>(mvs, kings[0]);
}

//------------------------------
// pseudo-legal quiet
//------------------------------
template<>
inline void Movegen::generate<quiet, pieces>() {
  // todo: order move generation by game phase
  // data-driven approach for this?
  generate<promotion, pawn>();
  generate<quiet, knight>();
  generate<quiet, bishop>();
  generate<quiet, rook>();
  generate<quiet, queen>();
  generate<quiet, pawn>();
  generate<quiet, king>();
  generate<castles, king>();
}

//------------------------------
// pseudo-legal capture
//------------------------------
template<>
inline void Movegen::generate<capture, pieces>() {
  // todo: order move generation by game phase
  // data-driven approach for this?
  generate<capture_promotion, pawn>();
  generate<capture, knight>();
  generate<capture, bishop>();
  generate<capture, rook>();
  generate<capture, queen>();
  generate<capture, pawn>();
  generate<capture, king>();
}

//------------------------------
// pseudo-legal all
//------------------------------

template<>
inline void Movegen::generate<pseudo_legal, pieces>() {
  // todo: order move generation by game phase
  // data-driven approach for this?
  if (check_target == 0ULL) { // all moves
    generate<capture_promotion, pawn>();
    generate<promotion, pawn>();
    generate<pseudo_legal, knight>();
    generate<pseudo_legal, bishop>();
    generate<pseudo_legal, rook>();
    generate<pseudo_legal, queen>();
    generate<capture, pawn>();
    generate<quiet, pawn>();
    generate<quiet, king>();
    generate<castles, king>();
    generate<capture, king>();
  }
  else if (check_target != 0ULL && evasion_target == 0ULL) {
    // step checker, only captures, and king evasions
      generate<capture_promotion, pawn>();
      generate<capture, king>();
      generate<capture, pawn>();
      generate<capture, knight>();
      generate<capture, bishop>();
      generate<capture, rook>();
      generate<capture, queen>();
      generate<quiet, king>();
  }
  else if (check_target != 0ULL && evasion_target != 0ULL) {
    if (!more_than_one(check_target)) {
      // capture moves
      generate<capture_promotion, pawn>();
      generate<capture, king>();
      generate<capture, pawn>();
      generate<pseudo_legal, knight>();
      generate<pseudo_legal, bishop>();
      generate<pseudo_legal, rook>();
      generate<pseudo_legal, queen>();
      
      // blocking moves
      generate<promotion, pawn>();
      generate<quiet, king>();
      generate<quiet, pawn>();
    }
    else {
      generate<quiet, king>();
      generate<capture, king>(); 
    }
  }
}
