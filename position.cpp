#include "position.h"
#include "move.h"

position::position(std::istringstream& fen) { setup(fen); }


position::position(const position& p) {
  std::copy(std::begin(p.history), std::end(p.history), std::begin(history));
  ifo = p.ifo;
  pcs = p.pcs;
  stats = p.stats;
  hidx = p.hidx;
  thread_id = p.thread_id;
  nodes_searched = p.nodes_searched;
}


position& position::operator=(const position& p) {
  std::copy(std::begin(p.history), std::end(p.history), std::begin(history));
  ifo = p.ifo;
  pcs = p.pcs;
  stats = p.stats;
  hidx = p.hidx;
  thread_id = p.thread_id;
  nodes_searched = p.nodes_searched;
  return *(this);
}


piece_data& piece_data::operator=(const piece_data& pd) {
  std::copy(std::begin(pd.bycolor), std::end(pd.bycolor), std::begin(bycolor));
  std::copy(std::begin(pd.king_sq), std::end(pd.king_sq), std::begin(king_sq));
  std::copy(std::begin(pd.color_on), std::end(pd.color_on), std::begin(color_on));
  std::copy(std::begin(pd.piece_on), std::end(pd.piece_on), std::begin(piece_on));
  std::copy(std::begin(pd.number_of), std::end(pd.number_of), std::begin(number_of));
  std::copy(std::begin(pd.bitmap), std::end(pd.bitmap), std::begin(bitmap));
  std::copy(std::begin(pd.piece_idx), std::end(pd.piece_idx), std::begin(piece_idx));
  std::copy(std::begin(pd.square_of), std::end(pd.square_of), std::begin(square_of));
  return (*this);
}


void position::setup(std::istringstream& fen) {
  clear();
  std::string token;
  fen >> token;
  Square s = Square::A8;
  
  for(auto& c : token) {
    if (isdigit(c)) s += int(c - '0');
    else if (c == '/') s -= 16;
    else { set_piece(c, s); ++s; }
  }
  
  // side to move
  fen >> token;
  ifo.stm = (token == "w" ? Color::white : Color::black);
  ifo.key ^= zobrist::stm(ifo.stm);
  
  // the castle rights
  fen >> token;  
  ifo.cmask = U16(0);
  for (auto& c : token) {
    U16 cr = CastleRights.at(c);
    ifo.cmask |= cr;
    ifo.key ^= zobrist::castle(ifo.stm, cr);
  }
  

  // ep square
  fen >> token;
  ifo.eps = Square::no_square;
  Row row = Row::no_row; Col col = Col::no_col;
  for (auto& c : token) {
    if (c >= 'a' && c <= 'h') col = Col(c - 'a');
    if (c == '3' || c == '6') row = Row(c - '1');
  }
  ifo.eps = Square(8 * row + col);
  if (ifo.eps != Square::no_square) ifo.key ^= zobrist::ep(util::col(ifo.eps));
  
  // half-moves since last pawn move/capture
  fen >> ifo.move50;
  ifo.key ^= zobrist::mv50(ifo.move50);
  
  // move counter
  fen >> ifo.hmvs;
  ifo.key ^= zobrist::hmvs(ifo.hmvs);

  // check info
  Color stm = to_move();
  ifo.ks[stm] = pcs.king_sq[stm];  
  ifo.incheck = is_attacked(ifo.ks[stm], stm, Color(stm^1));
  
  ifo.checkers = (in_check() ? attackers_of(ifo.ks[stm], Color(stm^1)) : 0ULL);
  ifo.pinned = pinned();
}

void position::do_move(const Move& m) {
  history[hidx++] = ifo;
  const Square from = Square(m.f); 
  const Square to = Square(m.t);
  const Movetype t = Movetype(m.type);
  const Piece p = piece_on(from);
  const Color us = to_move();

  // king square update and castle rights update
  if (p == king) {
    pcs.king_sq[us] = to;
    ifo.ks[us] = to;
    ifo.cmask &= (us == white ? clearw : clearb);
    ifo.key ^= (us == white ? zobrist::castle(white, clearw) : zobrist::castle(black, clearb));
  }
  else if (p == rook) {
    if (us == white) {
      if (from == A1) {
	ifo.cmask &= clearwqs;
	ifo.key ^= zobrist::castle(white, clearwqs);
      }
      else if (from == H1) {
	ifo.cmask &= clearwks;	
	ifo.key ^= zobrist::castle(white, clearwks);
      }
    }
    else {
      if (from == A8) {
	ifo.cmask &= clearbqs;
	ifo.key ^= zobrist::castle(white, clearbqs);
      }
      else if (from == H8) {
	ifo.cmask &= clearbks;
	ifo.key ^= zobrist::castle(white, clearbks);
      }
    }
  }

  ifo.captured = no_piece;
  
  if (t == quiet) {
    pcs.do_quiet(us, p, from, to, ifo);
  }
  
  else if (t == capture) {
    ifo.captured = piece_on(to); 
    pcs.do_cap(us, p, from, to, ifo);
  }

  else if (t == ep) {
    ifo.captured = pawn;
    pcs.do_ep(us, from, to, ifo);
  }
  
  else if (t < capture_promotion_q) pcs.do_promotion(us, (t == promotion_q ? queen :
							  t == promotion_r ? rook :
							  t == promotion_b ? bishop :
							  knight), from, to, ifo);
  
  else if (t < castle_ks) {
    ifo.captured = piece_on(to);
    pcs.do_promotion_cap(us, (t == capture_promotion_q ? queen :
			      t == capture_promotion_r ? rook :
			      t == capture_promotion_b ? bishop :
			      knight), from, to, ifo);
  }

  else if (t == castle_ks) {
    pcs.do_castle_ks(us, from, to, ifo);
    ifo.cmask &= (us == white ? clearw : clearb);
  }

  else if (t == castle_qs) {
    pcs.do_castle_qs(us, from, to, ifo);
    ifo.cmask &= (us == white ? clearw : clearb);
  }

  // eps
  ifo.eps = no_square;
  if (p == pawn && abs(from-to) == 16) {    
    ifo.eps = Square(from + (us == white ? 8 : -8));
    ifo.key ^= zobrist::ep(util::col(to));
  }

  // move50
  if (p == pawn || t == capture) ifo.move50 = 0;
  else ifo.move50++;
  ifo.key ^= zobrist::mv50(ifo.move50);

  // half-moves
  ifo.hmvs++;
  ifo.key ^= zobrist::hmvs(ifo.hmvs);
  
  // side to move
  ifo.stm = Color(ifo.stm ^ 1);
  ifo.key ^= zobrist::stm(ifo.stm);

  
  ifo.incheck = is_attacked(king_square(), ifo.stm, us);
  ifo.checkers = (ifo.incheck ? attackers_of(king_square(), Color(ifo.stm^1)) : 0ULL);
  ifo.pinned = pinned();
  ++nodes_searched;
}

void position::undo_move(const Move& m) {
  const Square from = Square(m.t); 
  const Square to = Square(m.f);
  const Movetype t = Movetype(m.type);
  const Piece p = piece_on(from);
  const Color us = Color(to_move()^1);
  Piece cp = ifo.captured;
  
  if (t == quiet) pcs.do_quiet(us, p, from, to, ifo);

  else if (t == capture) {
    pcs.do_quiet(us, p, from, to, ifo);
    pcs.add_piece(to_move(), cp, from, ifo);
  }

  else if (t == ep) {
    pcs.do_quiet(us, p, from, to, ifo);
    pcs.add_piece(to_move(), cp, Square(from + (us == white ? -8 : 8)), ifo);
  }
  
  else if (t < capture_promotion_q) {
    pcs.remove_piece(us, piece_on(from), from, ifo);
    pcs.add_piece(us, pawn, to, ifo);
  }
  
  else if (t < castle_ks) {
    pcs.remove_piece(us, piece_on(from), from, ifo);
    pcs.add_piece(to_move(), cp, from, ifo);
    pcs.add_piece(us, pawn, to, ifo);
  }

  else if (t == castle_ks) {
    Square rt = (us == white ? H1 : H8);
    Square rf = (us == white ? F1 : F8);
    pcs.do_quiet(us, king, from, to, ifo);
    pcs.do_quiet(us, rook, rf, rt, ifo);
  }

  else if (t == castle_qs) {
    Square rf = (us == white ? D1 : D8);
    Square rt = (us == white ? A1 : A8);
    pcs.do_quiet(us, king, from, to, ifo);
    pcs.do_quiet(us, rook, rf, rt, ifo);
  }
  ifo = history[--hidx];
}


void position::do_null_move() {
  const Color us = to_move();
  const Color them = Color(us^1);

  history[hidx++] = ifo;

  // eps square
  if (ifo.eps != Square::no_square) {
    ifo.key ^= zobrist::ep(util::col(ifo.eps));
    ifo.eps = Square::no_square;    
  }

  // side to move
  ifo.stm = them;
  ifo.key ^= zobrist::stm(ifo.stm);

  // move50
  ifo.move50++;
  ifo.key ^= zobrist::mv50(ifo.move50);

  // half-moves
  ifo.hmvs++;
  ifo.key ^= zobrist::hmvs(ifo.hmvs);
}


void position::undo_null_move() {
  ifo = history[--hidx]; 
}

bool position::is_legal_hashmove(const Move& m) {

  Square f = Square(m.f);
  Square t = Square(m.t);
  Piece p = piece_on(f);
  Color us = to_move();
  Color them = Color(us ^ 1);
  Square eks = king_square(them);
  bool slider = (p == rook || p == bishop || p == queen);
  Movetype mt = Movetype(m.type);

  if (f == t) return false;
  if (t == eks) return false;
  if (color_on(t) == us) return false;
  if (color_on(f) != us) return false;
  if ((mt == ep || mt == quiet || mt == promotion) && color_on(t) != Color::no_color) return false;
  if ((mt == capture ||
       mt == capture_promotion_q ||
       mt == capture_promotion_r ||
       mt == capture_promotion_b ||
       mt == capture_promotion_n) && color_on(t) != them) return false;
  if (mt == ep && t != ifo.eps) return false;
  
  if (!is_legal(m)) return false;
  
  if (p == pawn) {
    if (util::row_dist(f, t) != 1 && util::row_dist(f,t) != 2) return false;
    
    if (us == black && util::row(f) <= util::row(t)) return false;

    if (us == white && util::row(f) >= util::row(t)) return false;
    
    if (mt == quiet && util::col_dist(f, t) > 0) return false;
    
    if (mt == capture && (util::row_dist(f, t) != 1 || util::col_dist(f, t) != 1)) return false;        
  }

  if (p == knight) {
    int rd = util::row_dist(f, t);
    int cd = util::col_dist(f, t);
    if (std::min(rd, cd) != 1 || std::max(rd, cd) != 2) return false;
  }

  if (p == rook) {
    if (!util::same_row(f, t) && !util::same_col(f, t)) return false;
  }

  if (p == bishop) {
    if (!util::on_diagonal(f, t)) return false;
  }

  if (p == queen) {
    if (!util::same_row(f, t) && !util::same_col(f, t) && !util::on_diagonal(f, t)) return false;
  }

  if (p == king) {
    if (util::row_dist(f, t) != 1 || util::col_dist(f, t) != 1) return false;
  }
  
  if (slider) {
    U64 bb = bitboards::between[f][t];
    bb ^= bitboards::squares[f];
    bb ^= bitboards::squares[t];
    bb &= all_pieces();
    if (bits::count(bb) != 0) return false;
  }

  // if we are in check, hash move should either capture the checker
  // or block the check (king moves checked previously)
  if (in_check() && p != king) {
    U64 checks = checkers();
    if (bits::more_than_one(checks)) return false;

    Square check_f = Square(pop_lsb(checks));
    if (mt == capture && t != check_f) return false;

    Piece checker = piece_on(check_f);

    if (mt == quiet && (checker == pawn || checker == knight)) return false;
    
    if (checker == bishop || checker == rook || checker == queen) {
      U64 empty = ~all_pieces();
      U64 evasion_target = bitboards::between[check_f][king_square()] & empty;
      U64 block_bb = evasion_target & bitboards::squares[t];
      if (mt == quiet && block_bb == 0ULL) return false;
    }

  }
  
  return true;
}

bool position::is_legal(const Move& m) {
  Square f = Square(m.f);
  Square t = Square(m.t);
  Piece p = piece_on(f);
  Movetype mt = Movetype(m.type);
  Square ks = king_square();
  Color us = to_move();
  Color them = Color(us ^ 1);
  Square eks = king_square(them);
  auto pc = pcs.bitmap[them];

  // basic checks on hash moves
  if (f == t) return false;
  if (t == eks) return false;
  if (color_on(t) == us) return false;
  if (color_on(f) != us) return false;
  if ((mt == ep || mt == quiet || mt == promotion) && color_on(t) != Color::no_color) return false;
  if ((mt == capture || mt == capture_promotion) && color_on(t) != them) return false;
  
  // pinned
  if ((bitboards::squares[f] & ifo.pinned) && !util::aligned(ks, f, t)) return false;
  
  // ep can uncover a discovered check
  if (mt == ep) {
    Square csq = Square(t + (them == white ? 8 : -8));
    U64 msk = (all_pieces() ^ bitboards::squares[f] ^ bitboards::squares[csq]) |
      bitboards::squares[t];
    
    return (!(magics::attacks<bishop>(msk, ks) & (pc[queen] | pc[bishop])) &&
	    !(magics::attacks<rook>(msk, ks) & (pc[queen] | pc[rook])));
  }

  // can castle flag has already been checked in movegen
  if (mt == castle_ks || mt == castle_qs) {
    
    if (in_check()) return false;
    
    Square s1 = no_square;
    Square s2 = no_square;
    
    if (mt == castle_ks) {
      s1 = (us == white ? F1 : F8);
      s2 = (us == white ? G1 : G8);
      if (piece_on(us == white ? H1 : H8) != rook) return false;
    }
    else if (mt == castle_qs) {
      s1 = (us == white ? D1 : D8);
      s2 = (us == white ? C1 : C8);
      if (piece_on(us == white ? B1 : B8) != no_piece) return false;
      if (piece_on(us == white ? A1 : A8) != rook) return false;
    }
    
    if (piece_on(s1) != no_piece || piece_on(s2) != no_piece) return false;

    
    if (is_attacked(s1, us, them, all_pieces()) ||
	is_attacked(s2, us, them, all_pieces())) return false;

    return true;
  }
  
  // is the king move legal
  else if (p == king) {
    U64 msk = (all_pieces() ^ bitboards::squares[ks]);
    return !is_attacked(t, us, them, msk);
  }
  
  return true;
}

U64 position::pinned() {
  const Color us = to_move();
  const Color them = Color(us ^ 1);
  const Square ks = king_square();
  U64 pinned = 0ULL;
  auto p = pcs.bitmap[them];
  U64 sliders = ((p[bishop] | p[queen]) & bitboards::battks[ks]) |
    ((p[rook] | p[queen]) & bitboards::rattks[ks]);

  if (sliders == 0ULL) return pinned;
  do {
    int sq = pop_lsb(sliders);

    if (!util::aligned(sq, ks)) continue;
    
    U64 tmp = (bitboards::between[sq][ks] & all_pieces()) ^
      (bitboards::squares[ks] | bitboards::squares[sq]);
    
    if (!more_than_one(tmp)) pinned |= tmp;
    
  } while (sliders);
  
  return pinned & pcs.bycolor[us];
}

bool position::in_check() {
  return ifo.incheck;
}

bool position::is_attacked(const Square& s, const Color& us, const Color& them, U64 m) {
  // is square owned by "us" attacked by "them"
  auto p = pcs.bitmap[them];  
  U64 stepper_attacks = ((bitboards::pattks[us][s] & p[pawn]) |
			 (bitboards::nmask[s] & p[knight]) |
			 (bitboards::kmask[s] & p[king]));

  if (stepper_attacks != 0ULL) return true;
  
  if (m == 0ULL) m = all_pieces(); 

  return ((magics::attacks<bishop>(m, s) & (p[queen] | p[bishop]) ||
	   (magics::attacks<rook>(m, s) & (p[queen] | p[rook]))));
}

U64 position::attackers_of(const Square& s, const Color& c) {
  // attackers of square "s" by color "c"
  U64 m = all_pieces();
  auto p = pcs.bitmap[c];
  U64 battck = magics::attacks<bishop>(m, s);
  U64 rattck = magics::attacks<rook>(m, s);
  U64 qattck = battck | rattck;
  
  return ((bitboards::pattks[c^1][s] & p[pawn]) |
          (bitboards::nmask[s] & p[knight]) |
          (bitboards::kmask[s] & p[king]) |
	  (battck & p[bishop]) |
	  (rattck & p[rook]) |
	  (qattck & p[queen]));
}

void position::set_piece(const char& p, const Square& s) {
  auto idx = std::distance(SanPiece.begin(), std::find(SanPiece.begin(), SanPiece.end(), p));
  if (idx < 0) return; // error
  
  Color color = (idx < 6 ? white : black);
  Piece piece = Piece(idx < 6 ? idx : idx - 6);  
  pcs.set(color, piece, s, ifo);
  if (piece == king) ifo.ks[color] = s;  
}

void position::clear() {
  pcs.clear();
  stats.clear();
  hidx = 0;
  thread_id = 0;
  nodes_searched = 0;
  ifo = {};  
}

void position::print() const {
  std::cout << "   +---+---+---+---+---+---+---+---+" << std::endl;
  for (Row r = r8; r >= r1; --r) {    
    std::cout << " " << r + 1 << " ";
    
    for (Col c = A; c <= H; ++c) {
      Square s = Square(8 * r + c);
      if (pcs.piece_on[s] != no_piece) {
        Piece p = pcs.piece_on[s];
        std::cout << "| "
                  << (pcs.color_on[s] == Color::white ? SanPiece[p] : SanPiece[p+6])
                  << " ";
      }
      else std::cout << "|   ";	
    }
    std::cout << "|" << std::endl;
    std::cout << "   +---+---+---+---+---+---+---+---+" << std::endl;
  }
  std::cout << "     a   b   c   d   e   f   g   h  " << std::endl;
}
