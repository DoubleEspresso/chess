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


template<Movetype mt, Piece p>
inline void Movegen::encode(U64& b, const int& f) {  
  while (b) list[last++].set(p, U8(f), U8(pop_lsb(b)), Movetype(mt));
}

template<Movetype mt, Piece p>
inline void Movegen::encode_pawn_pushes(U64& b, const int& dir) {
  while (b) {
    int to = pop_lsb(b);
    list[last++].set(p, U8(to + dir), U8(to), Movetype(mt));
  }
}

template<Movetype mt, Piece p>
inline void Movegen::encode_promotions(U64& b, const int& dir) {
  while (b) {
    int frm = pop_lsb(b) + dir;
    int to = pop_lsb(b);
    list[last++].set(p, U8(frm), U8(to), Movetype(mt)); // queen
    list[last++].set(p, U8(frm), U8(to), Movetype(mt+2)); // rook
    list[last++].set(p, U8(frm), U8(to), Movetype(mt+4)); // bishop
    list[last++].set(p, U8(frm), U8(to), Movetype(mt+6)); // knight
  }
}

void Movegen::print() {
  for(int j=it; j<last; ++j) {    
    std::cout << list[j].to_string() << " ";
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
  
  for (int i=0; i<2; ++i) {
    s(dbl);
    dbl &= empty;
  }  
}


inline void Movegen::pawn_caps(U64& left, U64& right, U64& ep_left, U64& ep_right) {  
  // normal captures - non promotions
  left = pawns & pawnmaskleft[us];
  right = pawns & pawnmaskright[us];

  auto sw = (us == white ? shift<NW> : shift<SW>);
  auto se = (us == white ? shift<NE> : shift<SE>);
  
  if (left != 0ULL) {
    se(left);
    left &= enemies;
  }
  
  if (right != 0ULL) {
    sw(right);
    right &= enemies;
  }

  // ep captures
  if (eps == Square::no_square) return;
  auto r = (us == white ? Row::r5 : Row::r4);
  ep_left = pawns & pawnmaskleft[us] & row[r]; 
  ep_right = pawns & pawnmaskright[us] & row[r];
  
  if (ep_left != 0ULL) {
    sw(ep_left);
    ep_left &= bitboards::squares[eps];
  }
  
  if (ep_right != 0ULL) {
    se(ep_right);
    ep_right &= bitboards::squares[eps];
  }    
}

inline void Movegen::quiet_promotions(U64& quiets) {  
  if (pawns7 == 0ULL) return;
  quiets = pawns7;

  auto s = (us == white ? shift<N> : shift<S>);
  
  s(quiets);
}

inline void Movegen::capture_promotions(U64& right_caps, U64& left_caps) {
  U64 targets = enemies & (us == white ? row[Row::r8] : row[Row::r1]);
  right_caps = pawns;
  left_caps = pawns;

  auto se = (us == white ? shift<NE> : shift<SE>);
  se(right_caps);
  right_caps &= targets;

  auto sw = (us == white ? shift<NW> : shift<SW>);
  sw(left_caps);  
  left_caps &= targets;
}

inline void Movegen::knight_quiets(std::vector<U64>& mvs) {
  for (auto& s : knights) {
    if (s == Square::no_square) break;
    mvs.emplace_back((bitboards::nmask[s] & empty));
  }  
}

inline void Movegen::knight_caps(std::vector<U64>& mvs) {
  for (auto& s : knights) {
    if (s == Square::no_square) break;
    mvs.emplace_back((bitboards::nmask[s] & enemies));
  }
}

inline void Movegen::bishop_quiets(std::vector<U64>& mvs) {
  // optimization in case we called bishop_caps first
  if (bishop_mvs.size() > 0) {
    for (auto& m : bishop_mvs) {
      mvs.emplace_back((m & empty));
    }
    return;
  }

  // first time here - compute mvs from scratch
  U64 mask = all_pieces;
  for (auto& s : bishops) {
    if (s == Square::no_square) break;
    
    bishop_mvs.emplace_back(magics::attacks<bishop>(mask, s));
    mvs.emplace_back((bishop_mvs.back() & empty));
  }
}

inline void Movegen::bishop_caps(std::vector<U64>& mvs) {
  // optimization in case we called bishop_quiets first
  if (bishop_mvs.size() > 0) {
    for (auto& m : bishop_mvs) {
      mvs.emplace_back((m & enemies));
    }
    return;
  }

  // first time here, compute from scratch
  U64 mask = all_pieces;
  for (auto& s : bishops) {
    if (s == Square::no_square) break;
    
    bishop_mvs.emplace_back(magics::attacks<bishop>(mask, s));
    mvs.emplace_back((bishop_mvs.back() & enemies));
  }
}

inline void Movegen::rook_quiets(std::vector<U64>& mvs) {
  // optimization in case we called rook_quiets first
  if (rook_mvs.size() > 0) {
    for (auto& m : rook_mvs) {
      mvs.emplace_back((m & empty));
    }
    return;
  }

  // first time here - compute from scratch
  U64 mask = all_pieces;
  for (auto& s : rooks) {
    if (s == Square::no_square) break;
    rook_mvs.emplace_back(magics::attacks<rook>(mask, s));
    mvs.emplace_back((rook_mvs.back() & empty));
  }
}

inline void Movegen::rook_caps(std::vector<U64>& mvs) {
  // optimization in case we called rook_caps first
  if (rook_mvs.size() > 0) {
    for (auto& m : rook_mvs) {
      mvs.emplace_back((m & enemies));
    }
    return;
  }

  // first time here - compute from scratch
  U64 mask = all_pieces;
  for (auto& s : rooks) {
    if (s == Square::no_square) break;
    rook_mvs.emplace_back(magics::attacks<rook>(mask, s));
    mvs.emplace_back((rook_mvs.back() & enemies));
  }
}

inline void Movegen::queen_quiets(std::vector<U64>& mvs) {
  // optimization in case we called queen_caps first
  if (queen_mvs.size() > 0) {
    for (auto& m : queen_mvs) {
      mvs.emplace_back((m & empty));
    }
    return;
  }

  // first time here - compute from scratch
  U64 mask = all_pieces;
  for (auto& s : queens) {
    if (s == Square::no_square) break;
    queen_mvs.emplace_back((magics::attacks<bishop>(mask, s) |
			    magics::attacks<rook>(mask, s)));
    mvs.emplace_back((queen_mvs.back() & empty));
  }
}

inline void Movegen::queen_caps(std::vector<U64>& mvs) {
  // optimization in case we called queen_caps first
  if (queen_mvs.size() > 0) {
    for (auto& m : queen_mvs) {
      mvs.emplace_back((m & enemies));
    }
    return;
  }

  // first time here - compute from scratch
  U64 mask = all_pieces;
  for (auto& s : queens) {
    if (s == Square::no_square) break;
    queen_mvs.emplace_back((magics::attacks<bishop>(mask, s) |
			    magics::attacks<rook>(mask, s)));

    mvs.emplace_back((queen_mvs.back() & enemies));
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

inline void Movegen::castle_mvs(U64& mvs) {
  mvs |= (us == white ? bitboards::squares[G1] : bitboards::squares[G8]);
  mvs |= (us == white ? bitboards::squares[C1] : bitboards::squares[C8]);
}

//------------------------------
// pawn moves
//------------------------------

template<>
void Movegen::generate<quiet, pawn>() {
  
  U64 single_pushes = 0ULL;
  U64 double_pushes = 0ULL;
  int d1 = (us == white ? -8 : 8);
  int d2 = 2 * d1;
  
  pawn_quiets(single_pushes, double_pushes);

  if (single_pushes != 0ULL) encode_pawn_pushes<quiet, pawn>(single_pushes, d1);  
  if (double_pushes != 0ULL) encode_pawn_pushes<quiet, pawn>(double_pushes, d2);
}


template<>
void Movegen::generate<capture, pawn>() {
  U64 left = 0ULL;
  U64 right = 0ULL;
  U64 ep_left = 0ULL;
  U64 ep_right = 0ULL;
  
  int d1 = (us == white ? -9 : 9);
  int d2 = (us == white ? -7 : 7);
  
  pawn_caps(left, right, ep_left, ep_right);
  
  if (left != 0ULL) encode_pawn_pushes<capture, pawn>(left, d1);
  if (right != 0ULL) encode_pawn_pushes<capture, pawn>(right, d2);
  
  if (ep_left != 0ULL) encode_pawn_pushes<capture, pawn>(left, d1);  
  if (ep_right != 0ULL) encode_pawn_pushes<capture, pawn>(right, d2);
}

template<>
void Movegen::generate<promotion, pawn>() {
  U64 mvs = 0;
  quiet_promotions(mvs);
  encode_promotions<promotion, pawn>(mvs, us == white ? -8 : 8);  
}

template<>
void Movegen::generate<capture_promotion, pawn>() {
  U64 caps_l = 0;
  U64 caps_r = 0;
  int d1 = (us == white ? -9 : 9);
  int d2 = (us == black ? -7 : 7);
  capture_promotions(caps_r, caps_l);
  if (caps_r != 0ULL) encode_promotions<capture_promotion, pawn>(caps_r, d1);
  if (caps_l != 0ULL) encode_promotions<capture_promotion, pawn>(caps_l, d2);
}


//------------------------------
// knight moves
//------------------------------
template<>
void Movegen::generate<quiet, knight>() {
  std::vector<U64> mvs;
  
  knight_quiets(mvs);
  int idx = 0;
  for (auto& m : mvs) {
    if ( m != 0ULL) encode<quiet, knight>(m, knights[idx]);
    ++idx;
  }
}

template<>
void Movegen::generate<capture, knight>() {
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
void Movegen::generate<quiet, bishop>() {
  std::vector<U64> mvs;
  
  bishop_quiets(mvs);
  int idx = 0;
  for (auto& m : mvs) {
    if ( m != 0ULL) encode<quiet, bishop>(m, bishops[idx]);
    ++idx;
  }
}

template<>
void Movegen::generate<capture, bishop>() {
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
void Movegen::generate<quiet, rook>() {
  std::vector<U64> mvs;
  
  rook_quiets(mvs);
  int idx = 0;
  for (auto& m : mvs) {
    if ( m != 0ULL) encode<quiet, rook>(m, rooks[idx]);
    ++idx;
  }
}

template<>
void Movegen::generate<capture, rook>() {
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
void Movegen::generate<quiet, queen>() {
  std::vector<U64> mvs;
  
  queen_quiets(mvs);
  int idx = 0;
  for (auto& m : mvs) {
    if ( m != 0ULL) encode<quiet, queen>(m, queens[idx]);
    ++idx;
  }
}

template<>
void Movegen::generate<capture, queen>() {
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
void Movegen::generate<quiet, king>() {
  std::vector<U64> mvs;
  
  king_quiets(mvs);
  int idx = 0;
  for (auto& m : mvs) {
    if ( m != 0ULL) encode<quiet, king>(m, kings[idx]);
    ++idx;
  }
}

template<>
void Movegen::generate<capture, king>() {
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
void Movegen::generate<pseudo_legal_quiet, pieces>() {
  // todo: order move generation by game phase
  // data-driven approach for this?
  generate<promotion, pawn>();
  generate<quiet, knight>();
  generate<quiet, bishop>();
  generate<quiet, rook>();
  generate<quiet, queen>();
  generate<quiet, pawn>();
  generate<quiet, king>();  
}

//------------------------------
// pseudo-legal capture
//------------------------------
template<>
void Movegen::generate<pseudo_legal_capture, pieces>() {
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
void Movegen::generate<pseudo_legal_all, pieces>() {
  // todo: order move generation by game phase
  // data-driven approach for this?
  generate<pseudo_legal_capture, pieces>();
  generate<pseudo_legal_quiet, pieces>();
}

/*
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


//------------------------------
// queen
//------------------------------
template<>
void movegen<quiet, queen, white>::generate(const position& p) {  
  U64 mask = p.all_pieces();
  U64 empty = ~mask;
  for (auto& s : p.squares_of<white, queen>()) {
    if (s == Square::no_square) break;
    U64 mvs = (magics::attacks<bishop>(mask, s) |
	       magics::attacks<rook>(mask, s)) & empty;
    if (mvs != 0ULL) { encode(mvs, s); }    
  }
}

template<>
void movegen<capture, queen, white>::generate(const position& p) {  
  U64 mask = p.all_pieces();
  U64 enemies = p.get_pieces<black>();
  for (auto& s : p.squares_of<white, queen>()) {
    if (s == Square::no_square) break;
    U64 mvs = (magics::attacks<bishop>(mask, s) |
	       magics::attacks<rook>(mask, s)) & enemies;
    if (mvs != 0ULL) { encode(mvs, s); }    
  }
}

template<>
void movegen<quiet, queen, black>::generate(const position& p) {  
  U64 mask = p.all_pieces();
  U64 empty = ~mask;
  for (auto& s : p.squares_of<black, queen>()) {
    if (s == Square::no_square) break;
    U64 mvs = (magics::attacks<bishop>(mask, s) |
	       magics::attacks<rook>(mask, s)) & empty;
    if (mvs != 0ULL) { encode(mvs, s); }    
  }
}

template<>
void movegen<capture, queen, black>::generate(const position& p) {  
  U64 mask = p.all_pieces();
  U64 enemies = p.get_pieces<white>();
  for (auto& s : p.squares_of<black, queen>()) {
    if (s == Square::no_square) break;
    U64 mvs = (magics::attacks<bishop>(mask, s) |
	       magics::attacks<rook>(mask, s)) & enemies;
    if (mvs != 0ULL) { encode(mvs, s); }    
  }
}

//------------------------------
// king moves
//------------------------------
template<>
void movegen<quiet, king, white>::generate(const position& p) {  
  U64 empty = ~p.all_pieces();  
  for (auto& s : p.squares_of<white, king>()) {
    if (s == Square::no_square) break;    
    U64 mvs = bitboards::kmask[s] & empty;
    if (mvs != 0ULL) { encode(mvs, s); }    
  }
}

template<>
void movegen<capture, king, white>::generate(const position& p) {  
  U64 enemies = p.get_pieces<black>();  
  for (auto& s : p.squares_of<white, king>()) {
    if (s == Square::no_square) break;    
    U64 mvs = bitboards::kmask[s] & enemies;
    if (mvs != 0ULL) { encode(mvs, s); }    
  }
}

template<>
void movegen<quiet, king, black>::generate(const position& p) {  
  U64 empty = ~p.all_pieces();  
  for (auto& s : p.squares_of<black, king>()) {
    if (s == Square::no_square) break;    
    U64 mvs = bitboards::kmask[s] & empty;
    if (mvs != 0ULL) { encode(mvs, s); }    
  }
}

template<>
void movegen<capture, king, black>::generate(const position& p) {  
  U64 enemies = p.get_pieces<white>();  
  for (auto& s : p.squares_of<black, king>()) {
    if (s == Square::no_square) break;    
    U64 mvs = bitboards::kmask[s] & enemies;
    if (mvs != 0ULL) { encode(mvs, s); }    
  }
}



//--------------------------------------------------------
// legal/pseudo legal moves
//--------------------------------------------------------

template<>
void movegen<pseudo_legal, pieces, white>::generate(const position& p) {
  // todo: order based on phase
  if (p.to_move() == white) {
    
    generate<capture, knight, white>(p);    
    generate<capture, bishop, white>(p);
    generate<capture, rook, white>(p);
    generate<capture, queen, white>(p);
    generate<capture, pawn, white>(p);
    generate<capture, king, white>(p);

    generate<quiet, knight, white>(p);
    generate<quiet, bishop, white>(p);
    generate<quiet, rook, white>(p);
    generate<quiet, queen, white>(p);
    generate<quiet, pawn, white>(p);
    generate<quiet, king, white>(p);
  } else {
    generate<capture, knight, black>(p);
    generate<capture, bishop, black>(p);
    generate<capture, rook, black>(p);
    generate<capture, queen, black>(p);
    generate<capture, pawn, black>(p);
    generate<capture, king, black>(p);
    
    generate<quiet, knight, black>(p);
    generate<quiet, bishop, black>(p);
    generate<quiet, rook, black>(p);
    generate<quiet, queen, black>(p);
    generate<quiet, pawn, black>(p);
    generate<quiet, king, black>(p);
    
  }
}
*/
