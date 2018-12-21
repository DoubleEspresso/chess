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


template<Movetype mt, Piece p>
inline void Movegen::encode(U64& b, const int& f) {  
  while (b) list[last++] = Move(p, Square(f), Square(pop_lsb(b)), Movetype(mt));
}

template<Movetype mt, Piece p>
inline void Movegen::encode_pawn_pushes(U64& b, const int& dir) {
  while (b) {
    int to = pop_lsb(b);
    list[last++] = Move(p, Square(to + dir), Square(to), Movetype(mt));
  }
}

template<Movetype mt, Piece p>
inline void Movegen::encode_promotions(U64& b, const int& dir) {
  while (b) {
    Square to = Square(pop_lsb(b));
    Square frm = Square(to + dir);
    list[last++] = Move(p, frm, to, Movetype(mt)); // queen
    list[last++] = Move(p, frm, to, Movetype(mt+1)); // rook
    list[last++] = Move(p, frm, to, Movetype(mt+2)); // bishop
    list[last++] = Move(p, frm, to, Movetype(mt+3)); // knight
  }
}

inline void Movegen::print() {
  for(int j=it; j<last; ++j) {    
    std::cout << list[j].to_string() << " ";
  }
  std::cout << "\n";
}

inline void Movegen::print_legal(position& p) {
  for(int j=it; j<last; ++j) {    
    if (p.is_legal(list[j])) std::cout << list[j].to_string() << " ";
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

  // todo : refactor to one capture-target, and one quiet-target
  // check handling  
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

  eps = p.eps();
  pawns2 = pawns & rank2;
  pawns7 = pawns & rank7;
}


inline void Movegen::pawn_quiets(U64& single, U64& dbl) {
  single = pawns & pawnmask[us]; // filter the promotion candidates
  dbl = pawns & rank2;

  auto s = (us == white ? shift<N> : shift<S>);
  
  s(single);
  single &= empty;
  if (evasion_target != 0ULL) single &= evasion_target;
  
  for (int i=0; i<2; ++i) {
    s(dbl);
    dbl &= empty;
  }
  if (evasion_target != 0ULL) dbl &= evasion_target;
}


inline void Movegen::pawn_caps(U64& left, U64& right, U64& ep_left, U64& ep_right) {  
  // normal captures - non promotions
  left = pawns & pawnmaskleft[us];
  right = pawns & pawnmaskright[us];
 
  
  U64 target = (check_target == 0ULL ? enemies : check_target);
  
  auto sw = (us == white ? shift<NW> : shift<SE>);
  auto se = (us == white ? shift<NE> : shift<SW>);
  
  if (left != 0ULL) {
    se(left);
    left &= target;
  }
  
  if (right != 0ULL) {
    sw(right);
    right &= target;
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

inline void Movegen::quiet_promotions(U64& quiets) {  
  if (pawns7 == 0ULL) return;
  quiets = pawns7;

  auto s = (us == white ? shift<N> : shift<S>);
  
  s(quiets);

  quiets &= empty;
  
  if (evasion_target != 0ULL) quiets &= evasion_target;
}

inline void Movegen::capture_promotions(U64& right_caps, U64& left_caps) {
  if (pawns7 == 0ULL) return;
  
  U64 targets = enemies & (us == white ? row[Row::r8] : row[Row::r1]);
  if (check_target != 0ULL) targets &= check_target;

  right_caps = pawns7 & pawnmaskright[us^1];
  left_caps = pawns7 & pawnmaskleft[us^1];

  auto sw = (us == white ? shift<NW> : shift<SE>);
  auto se = (us == white ? shift<NE> : shift<SW>);

  if (left_caps) {
    se(left_caps);
    left_caps &= targets;
  }

  if (right_caps) {
    sw(right_caps);  
    right_caps &= targets;
  }
}

inline void Movegen::knight_quiets(std::vector<U64>& mvs) {
  for (auto& s : knights) {
    if (s == Square::no_square) break;
    if (evasion_target != 0ULL) mvs.emplace_back((bitboards::nmask[s] & evasion_target));
    else mvs.emplace_back((bitboards::nmask[s] & empty));
  }  
}

inline void Movegen::knight_caps(std::vector<U64>& mvs) {
  U64 target = (check_target == 0ULL ? enemies : check_target);
  for (auto& s : knights) {
    if (s == Square::no_square) break;
    mvs.emplace_back((bitboards::nmask[s] & target));
  }
}

inline void Movegen::bishop_quiets(std::vector<U64>& mvs) {
  U64 target = (check_target == 0ULL ? empty : evasion_target);
  // optimization in case we called bishop_caps first
  if (bishop_mvs.size() > 0) {
    for (auto& m : bishop_mvs) {
      mvs.emplace_back((m & target));
    }
    return;
  }

  // first time here - compute mvs from scratch
  U64 mask = all_pieces;
  for (auto& s : bishops) {
    if (s == Square::no_square) break;
    
    bishop_mvs.emplace_back(magics::attacks<bishop>(mask, s));
    mvs.emplace_back((bishop_mvs.back() & target));
  }
}

inline void Movegen::bishop_caps(std::vector<U64>& mvs) {
  U64 target = (check_target == 0ULL ? enemies : check_target);
  // optimization in case we called bishop_quiets first
  if (bishop_mvs.size() > 0) {
    for (auto& m : bishop_mvs) {
      mvs.emplace_back((m & target));
    }
    return;
  }

  // first time here, compute from scratch
  U64 mask = all_pieces;
  for (auto& s : bishops) {
    if (s == Square::no_square) break;
    
    bishop_mvs.emplace_back(magics::attacks<bishop>(mask, s));
    mvs.emplace_back((bishop_mvs.back() & target));
  }
}

inline void Movegen::rook_quiets(std::vector<U64>& mvs) {
  U64 target = (check_target == 0ULL ? empty : evasion_target);
  // optimization in case we called rook_quiets first
  if (rook_mvs.size() > 0) {
    for (auto& m : rook_mvs) {
      mvs.emplace_back((m & target));
    }
    return;
  }

  // first time here - compute from scratch
  U64 mask = all_pieces;
  for (auto& s : rooks) {
    if (s == Square::no_square) break;
    rook_mvs.emplace_back(magics::attacks<rook>(mask, s));
    mvs.emplace_back((rook_mvs.back() & target));
  }
}

inline void Movegen::rook_caps(std::vector<U64>& mvs) {
  U64 target = (check_target == 0ULL ? enemies : check_target);
  // optimization in case we called rook_caps first
  if (rook_mvs.size() > 0) {
    for (auto& m : rook_mvs) {
      mvs.emplace_back((m & target));
    }
    return;
  }

  // first time here - compute from scratch
  U64 mask = all_pieces;
  for (auto& s : rooks) {
    if (s == Square::no_square) break;
    rook_mvs.emplace_back(magics::attacks<rook>(mask, s));
    mvs.emplace_back((rook_mvs.back() & target));
  }
}

inline void Movegen::queen_quiets(std::vector<U64>& mvs) {
  U64 target = (check_target == 0ULL ? empty : evasion_target);
  // optimization in case we called queen_caps first
  if (queen_mvs.size() > 0) {
    for (auto& m : queen_mvs) {
      mvs.emplace_back((m & target));
    }
    return;
  }

  // first time here - compute from scratch
  U64 mask = all_pieces;
  for (auto& s : queens) {
    if (s == Square::no_square) break;
    queen_mvs.emplace_back((magics::attacks<bishop>(mask, s) |
			    magics::attacks<rook>(mask, s)));
    mvs.emplace_back((queen_mvs.back() & target));
  }
}

inline void Movegen::queen_caps(std::vector<U64>& mvs) {
  U64 target = (check_target == 0ULL ? enemies : check_target);
  // optimization in case we called queen_caps first
  if (queen_mvs.size() > 0) {
    for (auto& m : queen_mvs) {
      mvs.emplace_back((m & target));
    }
    return;
  }
  
  // first time here - compute from scratch
  U64 mask = all_pieces;
  for (auto& s : queens) {
    if (s == Square::no_square) break;
    queen_mvs.emplace_back((magics::attacks<bishop>(mask, s) |
			    magics::attacks<rook>(mask, s)));
    
    mvs.emplace_back((queen_mvs.back() & target));
  }
}

inline void Movegen::king_quiets(std::vector<U64>& mvs) {
  for (auto& s : kings) {
    if (s == Square::no_square) break;
    mvs.emplace_back((bitboards::kmask[s] & empty));
  }
}

inline void Movegen::king_caps(std::vector<U64>& mvs) {  
  for (auto& s : kings) {
    if (s == Square::no_square) break;
    mvs.emplace_back((bitboards::kmask[s] & enemies));
  }
}

//------------------------------
// pawn moves
//------------------------------

template<>
inline void Movegen::generate<quiet, pawn>() {
  
  U64 single_pushes = 0ULL;
  U64 double_pushes = 0ULL;
  int d1 = (us == white ? -8 : 8);
  int d2 = 2 * d1;
  
  pawn_quiets(single_pushes, double_pushes);

  if (single_pushes != 0ULL) encode_pawn_pushes<quiet, pawn>(single_pushes, d1);  
  if (double_pushes != 0ULL) encode_pawn_pushes<quiet, pawn>(double_pushes, d2);
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
  
  if (left != 0ULL) encode_pawn_pushes<capture, pawn>(left, d2);
  if (right != 0ULL) encode_pawn_pushes<capture, pawn>(right, d1);
  
  if (ep_left != 0ULL) encode_pawn_pushes<ep, pawn>(ep_left, d2);  
  if (ep_right != 0ULL) encode_pawn_pushes<ep, pawn>(ep_right, d1);  
}

template<>
inline void Movegen::generate<promotion, pawn>() {
  U64 mvs = 0;
  quiet_promotions(mvs);
  if (mvs) encode_promotions<promotion, pawn>(mvs, us == white ? -8 : 8);
}

template<>
inline void Movegen::generate<capture_promotion, pawn>() {
  U64 caps_l = 0;
  U64 caps_r = 0;
  int d1 = (us == white ? -7 : 9);
  int d2 = (us == white ? -9 : 7);
  capture_promotions(caps_r, caps_l);
  if (caps_r != 0ULL) encode_promotions<capture_promotion, pawn>(caps_r, d1);
  if (caps_l != 0ULL) encode_promotions<capture_promotion, pawn>(caps_l, d2);
}


//------------------------------
// knight moves
//------------------------------
template<>
inline void Movegen::generate<quiet, knight>() {
  std::vector<U64> mvs;
  
  knight_quiets(mvs);
  int idx = 0;
  for (auto& m : mvs) {
    if ( m != 0ULL) encode<quiet, knight>(m, knights[idx]);
    ++idx;
  }
}

template<>
inline void Movegen::generate<capture, knight>() {
  std::vector<U64> mvs;
  
  knight_caps(mvs);
  int idx = 0;
  for (auto& m : mvs) {
    if ( m != 0ULL) encode<capture, knight>(m, knights[idx]);
    ++idx;
  }
}

//------------------------------
// bishop moves
//------------------------------
template<>
inline void Movegen::generate<quiet, bishop>() {
  std::vector<U64> mvs;
  
  bishop_quiets(mvs);
  int idx = 0;
  for (auto& m : mvs) {
    if ( m != 0ULL) encode<quiet, bishop>(m, bishops[idx]);
    ++idx;
  }
}

template<>
inline void Movegen::generate<capture, bishop>() {
  std::vector<U64> mvs;
  
  bishop_caps(mvs);
  int idx = 0;
  for (auto& m : mvs) {
    if ( m != 0ULL) encode<capture, bishop>(m, bishops[idx]);
    ++idx;
  }
}

//------------------------------
// rook moves
//------------------------------
template<>
inline void Movegen::generate<quiet, rook>() {
  std::vector<U64> mvs;
  
  rook_quiets(mvs);
  int idx = 0;
  for (auto& m : mvs) {
    if ( m != 0ULL) encode<quiet, rook>(m, rooks[idx]);
    ++idx;
  }
}

template<>
inline void Movegen::generate<capture, rook>() {
  std::vector<U64> mvs;
  
  rook_caps(mvs);
  int idx = 0;
  for (auto& m : mvs) {
    if ( m != 0ULL) encode<capture, rook>(m, rooks[idx]);
    ++idx;
  }
}

//------------------------------
// queen moves
//------------------------------
template<>
inline void Movegen::generate<quiet, queen>() {
  std::vector<U64> mvs;
  
  queen_quiets(mvs);
  int idx = 0;
  for (auto& m : mvs) {
    if ( m != 0ULL) encode<quiet, queen>(m, queens[idx]);
    ++idx;
  }
}

template<>
inline void Movegen::generate<capture, queen>() {
  std::vector<U64> mvs;
  
  queen_caps(mvs);
  int idx = 0;
  for (auto& m : mvs) {
    if ( m != 0ULL) encode<capture, queen>(m, queens[idx]);
    ++idx;
  }
}

//------------------------------
// king moves
//------------------------------
template<>
inline void Movegen::generate<quiet, king>() {
  std::vector<U64> mvs;
  
  king_quiets(mvs);
  int idx = 0;
  for (auto& m : mvs) {
    if ( m != 0ULL) encode<quiet, king>(m, kings[idx]);
    ++idx;
  }
}

template<>
inline void Movegen::generate<castles, king>() {
  const Square f = (us == white ? E1 : E8);
  if (can_castle_ks) list[last++] = Move(king, f, (us == white ? G1 : G8), castle_ks);
  if (can_castle_qs) list[last++] = Move(king, f, (us == white ? C1 : C8), castle_qs);
}

template<>
inline void Movegen::generate<capture, king>() {
  std::vector<U64> mvs;
  
  king_caps(mvs);
  int idx = 0;
  for (auto& m : mvs) {
    if ( m != 0ULL) encode<capture, king>(m, kings[idx]);
    ++idx;
  }
}

//------------------------------
// pseudo-legal quiet
//------------------------------
template<>
inline void Movegen::generate<pseudo_legal_quiet, pieces>() {
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
inline void Movegen::generate<pseudo_legal_capture, pieces>() {
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
inline void Movegen::generate<pseudo_legal_all, pieces>() {
  // todo: order move generation by game phase
  // data-driven approach for this?

  if (check_target == 0ULL) {    
    generate<pseudo_legal_capture, pieces>();
    generate<pseudo_legal_quiet, pieces>();
  }
  else if (check_target != 0ULL && evasion_target == 0ULL) {
    // step checker, only captures, and king evasions
    if (!more_than_one(check_target)) {
      generate<capture_promotion, pawn>();
      generate<capture, king>();
      generate<capture, pawn>();
      generate<capture, knight>();
      generate<capture, bishop>();
      generate<capture, rook>();
      generate<capture, queen>();
    }
    generate<quiet, king>();
  }
  else if (check_target != 0ULL && evasion_target != 0ULL) {
    if (!more_than_one(check_target)) {
      // capture moves
      generate<capture_promotion, pawn>();
      generate<capture, king>();
      generate<capture, pawn>();
      generate<capture, knight>();
      generate<capture, bishop>();
      generate<capture, rook>();
      generate<capture, queen>();
      
      // blocking moves
      generate<promotion, pawn>();
      generate<quiet, king>();
      generate<quiet, pawn>();
      generate<quiet, knight>();
      generate<quiet, bishop>();
      generate<quiet, rook>();
      generate<quiet, queen>();

    }
    else generate<quiet, king>(); // more than one checker
  }
}
