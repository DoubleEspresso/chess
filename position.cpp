#include "position.h"
#include "move.h"

position::position(std::istringstream& fen) { setup(fen); }
						    
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

  // the castle rights
  fen >> token;
  
  ifo.cmask = U16(0);
  for (auto& c : token) ifo.cmask |= CastleRights.at(c);

  // ep square
  fen >> token;
  ifo.eps = Square::no_square;
  Row row = Row::no_row; Col col = Col::no_col;
  for (auto& c : token) {
    if (c >= 'a' && c <= 'h') col = Col(c - 'a');
    if (c == '3' || c == '6') row = Row(c - '1');
  }
  ifo.eps = Square(8 * row + col);

  // half-moves since last pawn move/capture
  fen >> ifo.move50;
  
  // move counter
  fen >> ifo.hmvs;

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
  const Movetype t = m.type;
  const Piece p = piece_on(from);
  const Color us = to_move();

  // king square update and castle rights update
  if (p == king) {
    pcs.king_sq[us] = to;
    ifo.ks[us] = to;
    ifo.cmask &= (us == white ? clearw : clearb);
  }
  else if (p == rook) {
    if (us == white) {
      if (from == A1) ifo.cmask &= clearwqs;
      else if (from == H1) ifo.cmask &= clearwks;      
    }
    else {
      if (from == A8) ifo.cmask &= clearbqs;
      else if (from == H8) ifo.cmask &= clearbks;      
    }
  }
  
  ifo.captured = no_piece;
  
  if (t == quiet) pcs.do_quiet(us, p, from, to);
  
  else if (t == capture) {
    ifo.captured = piece_on(to); 
    pcs.do_cap(us, p, from, to);
  }

  else if (t == ep) {
    ifo.captured = pawn;
    pcs.do_ep(us, from, to);
  }

  else if (t < capture_promotion) pcs.do_promotion(us, (t == promotion_q ? queen :
							t == promotion_r ? rook :
							t == promotion_b ? bishop :
							knight), from, to);
  
  else if (t < castle_ks) {
    ifo.captured = piece_on(to);
    pcs.do_promotion_cap(us, (t == capture_promotion_q ? queen :
			      t == capture_promotion_r ? rook :
			      t == capture_promotion_b ? bishop :
			      knight), from, to);
  }

  else if (t == castle_ks) {
    pcs.do_castle_ks(us, from, to);
    ifo.cmask &= (us == white ? clearw : clearb);
  }

  else if (t == castle_qs) {
    pcs.do_castle_qs(us, from, to);
    ifo.cmask &= (us == white ? clearw : clearb);
  }

  // eps
  ifo.eps = no_square;
  if (p == pawn && abs(from-to) == 16) {    
    ifo.eps = Square(from + (us == white ? 8 : -8));
    // todo: zobrist
  }

  // move50
  if (p == pawn || t == capture) ifo.move50 = 0;
  else ifo.move50++;

  // half-moves
  ifo.hmvs++;

  // side to move
  ifo.stm = Color(ifo.stm ^ 1);

  ifo.incheck = is_attacked(king_square(), ifo.stm, us);
  ifo.checkers = (ifo.incheck ? attackers_of(king_square(), Color(ifo.stm^1)) : 0ULL);
  ifo.pinned = pinned();
}

void position::undo_move(const Move& m) {
  const Square from = Square(m.t); 
  const Square to = Square(m.f);
  const Movetype t = m.type;
  const Piece p = piece_on(from);
  const Color us = Color(to_move()^1);
  Piece cp = ifo.captured;
  
  if (t == quiet) pcs.do_quiet(us, p, from, to);

  else if (t == capture) {
    pcs.do_quiet(us, p, from, to);
    pcs.add_piece(to_move(), cp, from);
  }

  else if (t == ep) {
    pcs.do_quiet(us, p, from, to);
    pcs.add_piece(to_move(), cp, Square(from + (us == white ? -8 : 8)));
  }

  else if (t < capture_promotion) {
    pcs.remove_piece(us, piece_on(from), from); // promoted piece
    pcs.add_piece(us, pawn, to);
  }
  
  else if (t < castle_ks) {
    pcs.remove_piece(us, piece_on(from), from);
    pcs.add_piece(to_move(), cp, from);
    pcs.add_piece(us, pawn, to);
  }

  else if (t == castle_ks) {
    Square rt = (us == white ? H1 : H8);
    Square rf = (us == white ? F1 : F8);
    pcs.do_quiet(us, king, from, to);
    pcs.do_quiet(us, rook, rf, rt);
  }

  else if (t == castle_qs) {
    Square rf = (us == white ? D1 : D8);
    Square rt = (us == white ? A1 : A8);
    pcs.do_quiet(us, king, from, to);
    pcs.do_quiet(us, rook, rf, rt);
  }
  ifo = history[--hidx];
}

bool position::is_legal(const Move& m) {
  Square f = Square(m.f);
  Square t = Square(m.t);
  Piece p = piece_on(f);
  Movetype mt = m.type;
  Square ks = king_square();
  Color us = to_move();
  Color them = Color(us ^ 1);
  //auto pc = pcs.bitmap[them];
  
  // ep can uncover a discovered check 
  if (mt == ep) {
    U64 sliders = pcs.bitmap[them][queen] | pcs.bitmap[them][rook] | pcs.bitmap[them][bishop];
    if (!sliders) return true;
    
    Square csq = Square(t + (them == white ? 8 : -8));
    U64 msk = (all_pieces() ^ bitboards::squares[f] ^ bitboards::squares[csq]) |
      bitboards::squares[t];
    
    return (!(magics::attacks<bishop>(msk, ks) & (pcs.bitmap[them][queen] | pcs.bitmap[them][bishop])) &&
	    !(magics::attacks<rook>(msk, ks) & (pcs.bitmap[them][queen] | pcs.bitmap[them][rook])));
  }

  // can castle flag has already been checked in movegen
  else if (mt == castle_ks || mt == castle_qs) {

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

  // pinned
  else if ((bitboards::squares[f] & ifo.pinned) && !util::aligned(ks, f, t)) return false;

  
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
  pcs.set(color, piece, s);
  if (piece == king) ifo.ks[color] = s;
    
  // update zobrist keys
  // update material entries
  // update pawn keys
}

void position::clear() {
  pcs.clear();
  hidx = 0;
  ifo = {};
}

void position::print() {
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
