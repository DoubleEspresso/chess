#include <algorithm>
#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstring>

#include "board.h"
#include "globals.h"
#include "bits.h"
#include "magic.h"
#include "material.h"
#include "zobrist.h"
#include "move.h"
#include "uci.h"

#include <cassert>

using namespace Globals;
using namespace Magic;
Board ROOT_BOARD;
int ROOT_DEPTH;

Board::Board() : position(0), thread(0) { }
Board::Board(std::istringstream& is) : position(0), thread(0) { from_fen(is); }
Board::Board(Board& b) : position(0), thread(0) { memcpy(this, &b, sizeof(Board)); }
Board::~Board() { }

std::string Board::to_fen() {
  std::string fen = "";
  for (int r = 7; r >= 0; --r) {
    int empties = 0;

    for (int c = 0; c < 8; ++c) {
      int s = r * 8 + c;
      if (piece_on(s) == PIECE_NONE) { ++empties; continue; }
      if (empties > 0) fen += std::to_string(empties);
      empties = 0;
      
      fen += SanPiece[(color_on(s) == BLACK ? piece_on(s) + 6 : piece_on(s))];
    }
    if (empties > 0) fen += std::to_string(empties);   
    if (r > 0) fen += "/";
  }

  fen += (whos_move() == WHITE ? " w" : " b");

  // castle rights
  std::string castleRights = "";
  int crights = position->crights;
  if ((crights & W_KS) == W_KS) castleRights += "K";
  if ((crights & W_QS) == W_QS) castleRights += "Q";
  if ((crights & B_KS) == B_KS) castleRights += "k";
  if ((crights & B_QS) == B_QS) castleRights += "q";
  fen += (castleRights == "" ? " -" : " " + castleRights);

  // ep-square
  std::string epSq = "";
  if (position->eps != 0) {
    epSq += SanCols[COL(position->eps)] + std::to_string(ROW(position->eps) + 1);
  }
  fen += (epSq == "" ? " -" : " " + epSq);

  // move50
  std::string mv50 = std::to_string(position->move50);
  fen += " " + mv50;

  // half-mvs
  std::string half_mvs = std::to_string(position->hmvs);
  fen += " " + half_mvs;

  return fen;
}
void Board::from_fen(std::istringstream& is)
{
  std::string token;
  clear();
  is >> token;
  int s = A8;

  // main parse of the incoming fen string, will loop over all squares
  // and set the pieces/game data 
  for (std::string::iterator it = token.begin(), end = token.end(); it != end; ++it)
    {
      char c = *it;
      if (isdigit(c)) s += int(c - '0');
      else
	switch (c)
	  {
	  case '/':
	    s -= 16;
	    break;
	  default:
	    set_piece(c, s);
	    s++;
	    break;
	  }
    }

  // the side to move
  is >> token;
  position->stm = (token == "w" ? WHITE : BLACK);

  // set the king square 
  position->ks = lsb(king_sq_bm[position->stm]);
  position->king_square[WHITE] = lsb(king_sq_bm[WHITE]);
  position->king_square[BLACK] = lsb(king_sq_bm[BLACK]);

  // the castle rights
  is >> token;
  position->crights = U16(0);
  for (std::string::iterator it = token.begin(), end = token.end(); it != end; ++it)
    {
      char c = *it;
      switch (c)
	{
	case 'K':
	  position->crights |= W_KS;
	  position->dKey ^= Zobrist::castle_rands(WHITE, W_KS);
	  break;
	case 'Q':
	  position->crights |= W_QS;
	  position->dKey ^= Zobrist::castle_rands(WHITE, W_QS);
	  break;
	case 'k':
	  position->crights |= B_KS;
	  position->dKey ^= Zobrist::castle_rands(BLACK, B_KS);
	  break;
	case 'q':
	  position->crights |= B_QS;
	  position->dKey ^= Zobrist::castle_rands(BLACK, B_QS);
	  break;
	}
    }
  // the ep-square
  is >> token;

  int row = ROW_NONE;
  int col = COL_NONE;
  if (token != "-")
    {
      for (std::string::iterator it = token.begin(), end = token.end(); it != end; ++it)
	{
	  char c = *it;
	  if (c >= 'a' && c <= 'h') col = int(c - 'a');
	  if (c == '3' || c == '6') row = int(c - '1');
	}
      position->eps = 8 * row + col;
      position->dKey ^= Zobrist::ep_rands(col);
    }
  else position->eps = SQUARE_NONE;

  // the half-moves since last pawn move/capture
  is >> position->move50;
  position->dKey ^= Zobrist::mv50_rands(position->move50);

  // the move counter
  is >> position->hmvs;
  position->dKey ^= position->hmvs;

  //-------------------------------------
  // additional setup routines
  // 1. check if king in check, pinned pieces to king
  // 2. hash keys
  // 3. ownership to main_thread()
  // 4. check that the position is legal
  //-------------------------------------
  // zobrist keys
  position->dKey ^= Zobrist::stm_rands(position->stm);

  // check info
  position->in_check = is_attacked(position->ks, position->stm, position->stm ^ 1);  
  (position->in_check ? position->checkers = (attackers_of(position->ks) & colored_pieces(position->stm ^ 1)) : 0ULL);
}
void Board::do_null_move(BoardData& d)
{
  int us = position->stm;
  int them = (us == WHITE ? BLACK : WHITE);

  std::memcpy(&d, position, sizeof(BoardDataReduced));
  d.previous = position;
  position = &d;

  // the eps
  if (position->eps != SQUARE_NONE)
    {
      position->dKey ^= Zobrist::ep_rands(COL(position->eps));
      position->eps = SQUARE_NONE;
    }

  // the stm
  position->dKey ^= Zobrist::stm_rands(us);
  position->stm = them;

  // move50
  position->move50++;
  position->dKey ^= Zobrist::mv50_rands(position->move50);

  // half-moves
  position->hmvs++;
  position->dKey ^= position->hmvs;

  //king sq.
  position->ks = square_of_arr[position->stm][KING][1];
  position->king_square[WHITE] = square_of_arr[WHITE][KING][1];
  position->king_square[BLACK] = square_of_arr[BLACK][KING][1];
}
void Board::undo_null_move()
{
  position = position->previous;
}
void Board::do_move(BoardData& d, U16 m, bool qsMove)
{
  int from = int(m & 0x3f);
  int to = int((m & 0xfc0) >> 6);
  int type = int((m & 0xf000) >> 12);
  int p = piece_on_arr[from];

  int us = position->stm;
  int them = (us == WHITE ? BLACK : WHITE);
  int idx = piece_index[us][p][from];
  U64 fbit = SquareBB[from];
  U64 tbit = SquareBB[to];
  U64 tfbit = SquareBB[from] | SquareBB[to];

  std::memcpy(&d, position, sizeof(BoardDataReduced));
  d.previous = position;
  position = &d;
  nodes_searched++;
  (qsMove ? qsnodes++ : msnodes++);

  if(from == position->ks)
    {
      position->ks = to;
      (us == WHITE ? position->crights &= 0xC : position->crights &= 0x3);
      position->king_square[us] = to;
    }
  if(p == ROOK && (SquareBB[from] & CornersBB))
    {
      if (from == H1) { unset_castle_rights(W_KS); position->dKey ^= Zobrist::castle_rands(WHITE, W_KS); }
      else if (from == A1) { unset_castle_rights(W_QS); position->dKey ^= Zobrist::castle_rands(WHITE, W_QS); }
      else if (from == H8) { unset_castle_rights(B_KS); position->dKey ^= Zobrist::castle_rands(BLACK, B_KS); }
      else if (from == A8) { unset_castle_rights(B_QS); position->dKey ^= Zobrist::castle_rands(BLACK, B_QS); }
    }
  position->captured = PIECE_NONE;

  if (type == QUIET)
    {
      move_piece(us, p, idx, from, to);
      pieces[us][p] ^= tfbit;
      pieces_by_color[us] ^= tfbit;
    }
  else if (type == CAPTURE)
    {
      position->captured = Piece(piece_on_arr[to]);
      remove_piece(them, position->captured, to);
      move_piece(us, p, idx, from, to);
      pieces[us][p] ^= tfbit;
      pieces_by_color[us] ^= tfbit;
      pieces[them][position->captured] ^= tbit;
      pieces_by_color[us == WHITE ? BLACK : WHITE] ^= tbit;
    }
  // ep moves here
  else if(type == EP)
    {
      int tmp = to + (us == WHITE ? SOUTH : NORTH);
      position->captured = PAWN;
      remove_piece(them, PAWN, tmp);
      move_piece(us, PAWN, idx, from, to);

      pieces[them][PAWN] ^= SquareBB[tmp];
      pieces_by_color[us == WHITE ? BLACK : WHITE] ^= SquareBB[tmp];
      pieces[us][p] ^= tfbit;
      pieces_by_color[us] ^= tfbit;
    }
  // normal promotion moves (no captures)
  else if(type != MOVE_NONE && type <= PROMOTION)
    {
      Piece pp = Piece(type);
      remove_piece(us, PAWN, from);
      add_piece(us, pp, to);
      pieces[us][p] ^= fbit;
      pieces[us][type] |= tbit;
      pieces_by_color[us] ^= tfbit;
    }
  // capture promotion moves 
  else if(type > PROMOTION && type <= PROMOTION_CAP)
    {
      position->captured = Piece(piece_on_arr[to]);
      Piece pp = Piece(type - 4);

      pieces[them][piece_on_arr[to]] ^= tbit;
      pieces_by_color[us == WHITE ? BLACK : WHITE] ^= tbit;
      pieces[us][p] ^= fbit;
      pieces[us][type - 4] |= tbit;
      pieces_by_color[us] ^= tfbit;

      remove_piece(us, PAWN, from);
      remove_piece(them, position->captured, to);
      add_piece(us, pp, to);
    }
  // castle moves
  else if(type == CASTLE_KS || type == CASTLE_QS)
    {
      int rook_from, rook_to;
      if (type == CASTLE_KS)
	{
	  rook_from = (us == WHITE ? H1 : H8);
	  rook_to = (us == WHITE ? F1 : F8);
	}
      else
	{
	  rook_from = (us == WHITE ? A1 : A8);
	  rook_to = (us == WHITE ? D1 : D8);
	}
      move_piece(us, KING, idx, from, to);
      pieces[us][p] ^= tfbit;
      pieces_by_color[us] ^= tfbit;

      int rook_idx = piece_index[us][ROOK][rook_from];
      move_piece(us, ROOK, rook_idx, rook_from, rook_to);

      // update the rook bitboard
      U64 rook_from_to = (SquareBB[rook_from] | SquareBB[rook_to]);
      pieces[us][ROOK] ^= rook_from_to;
      pieces_by_color[us] ^= rook_from_to;

      // unset the castle rights here
      if (type == CASTLE_KS || type == CASTLE_QS)
	{
	  (us == WHITE ? position->crights &= 0xC : position->crights &= 0x3);
	  // update the castle zobrist keys
	  if (us == WHITE) position->dKey ^= (Zobrist::castle_rands(WHITE, W_KS) | Zobrist::castle_rands(WHITE, W_QS));
	  else position->dKey ^= (Zobrist::castle_rands(BLACK, B_KS) | Zobrist::castle_rands(BLACK, B_QS));
	  castled[us] = true;
	}
    }
  // updates -- eps, capturedPiece, pinned, in_check, stm
  position->eps = SQUARE_NONE;
  if (p == PAWN && (from + Square((us == WHITE ? (NORTH + NORTH) : (SOUTH + SOUTH)))) == to)
    {
      position->eps = from + (us == WHITE ? NORTH : SOUTH);
      position->dKey ^= Zobrist::ep_rands(COL(to)); // have to undo this
    }
  // move50
  if (p == PAWN || type == CAPTURE) position->move50 = 0;
  else position->move50++;
  position->dKey ^= Zobrist::mv50_rands(position->move50);

  // half-moves
  position->hmvs++;
  position->dKey ^= position->hmvs;

  // side to move
  position->stm ^= 1;
  position->dKey ^= Zobrist::stm_rands(position->stm);

  position->ks = square_of_arr[position->stm][KING][1];
  position->king_square[WHITE] = square_of_arr[WHITE][KING][1];
  position->king_square[BLACK] = square_of_arr[BLACK][KING][1];

  // if in check, set the checkers bitmap
  position->in_check = is_attacked(position->ks, position->stm, us);
  position->checkers = (position->in_check ? attackers_of(position->ks) & colored_pieces(position->stm ^ 1) : 0ULL);

  // update discovered checking candidates
  //compute_discovered_candidates(position->stm);
  //compute_discovered_candidates(position->stm ^ 1);
}
void Board::undo_move(U16 m)
{
  int to = int(m & 0x3f);
  int from = int((m & 0xfc0) >> 6);
  int type = int((m & 0xf000) >> 12);
  int p = piece_on_arr[from];
  int them = position->stm;
  int us = (them == WHITE ? BLACK : WHITE);
  U64 fbit = SquareBB[from];
  U64 tbit = SquareBB[to];
  U64 tfbit = SquareBB[from] | SquareBB[to];
  int idx = piece_index[us][p][from];
  // quite moves
  if (type == QUIET)
    {
      move_piece(us, p, idx, from, to);
      pieces[us][p] ^= tfbit;
      pieces_by_color[us] ^= tfbit;
    }
  else if (type == CAPTURE)
    {
      Piece cp = position->captured;
      move_piece(us, p, idx, from, to);
      add_piece(them, cp, from);
      pieces[us][p] ^= tfbit;
      pieces_by_color[us == WHITE ? BLACK : WHITE] |= fbit;
      pieces[them][cp] |= fbit;
      pieces_by_color[us] ^= tfbit;
    }
  else if(type == EP)
    {
      int tmp = from + (us == WHITE ? SOUTH : NORTH);
      U64 tmpbb = SquareBB[tmp];
      Piece cp = PAWN;

      move_piece(us, PAWN, idx, from, to);
      add_piece(them, PAWN, tmp);
      pieces[them][PAWN] ^= tmpbb;
      pieces_by_color[us == WHITE ? BLACK : WHITE] ^= tmpbb;

      pieces[us][p] ^= tfbit;
      pieces[them][cp] |= tmpbb;
      pieces_by_color[us == WHITE ? BLACK : WHITE] |= tmpbb;
      pieces_by_color[us] ^= tfbit;
    }
  // normal promotion moves (no captures)
  else if(type != MOVE_NONE && type <= PROMOTION)
    {
      Piece pp = Piece(piece_on_arr[from]);
      remove_piece(us, pp, from);
      add_piece(us, PAWN, to);
      pieces[us][pp] ^= fbit;
      pieces[us][PAWN] |= tbit;
      pieces_by_color[us] ^= tfbit;
    }
  // capture promotion moves 
  else if (type > PROMOTION && type <= PROMOTION_CAP)
    {
      Piece pp = Piece(piece_on_arr[from]);
      Piece cp = position->captured;
      pieces[them][cp] |= fbit;
      pieces_by_color[us == WHITE ? BLACK : WHITE] |= fbit;
      pieces[us][pp] ^= fbit;  pieces[us][PAWN] |= tbit;
      pieces_by_color[us] ^= tfbit;
      remove_piece(us, pp, from);
      add_piece(us, PAWN, to);
      add_piece(them, cp, from);
    }
  // castle moves
  else if(type == CASTLE_KS || type == CASTLE_QS)
    {
      int rook_from, rook_to;
      if (type == CASTLE_KS)
	{
	  rook_to = (us == WHITE ? H1 : H8);
	  rook_from = (us == WHITE ? F1 : F8);
	}
      else
	{
	  rook_to = (us == WHITE ? A1 : A8);
	  rook_from = (us == WHITE ? D1 : D8);
	}
      move_piece(us, KING, idx, from, to);
      int rook_idx = piece_index[us][ROOK][rook_from];
      move_piece(us, ROOK, rook_idx, rook_from, rook_to);
      pieces[us][p] ^= tfbit;
      pieces_by_color[us] ^= tfbit;

      U64 rook_to_from = SquareBB[rook_from] | SquareBB[rook_to];
      pieces[us][ROOK] ^= rook_to_from;
      pieces_by_color[us] ^= rook_to_from;
      castled[us] = false;
    }
  // copy the position state back to previous
  position = position->previous;
}
bool Board::is_quiet(U16& move)
{
  int mt = ((move & 0xf000) >> 12);
  return (mt != EP && mt != CAPTURE && mt != PROMOTION_CAP);
}
bool Board::is_qsearch_mv(U16& move)
{
  int mt = ((move & 0xf000) >> 12);
  return mt != MOVE_NONE && (mt == EP || mt == CAPTURE || mt <= PROMOTION || (mt > PROMOTION && mt <= PROMOTION_CAP));
}
bool Board::is_promotion(const U16& m)
{
  int type = ((m & 0xf000) >> 12);
  return ((type != MOVE_NONE && type <= PROMOTION) ||
	  (type > PROMOTION && type <= PROMOTION_CAP));
}
bool Board::is_legal(U16& move)
{
  // note: assumed move is made by color = position->stm
  if (move == MOVE_NONE) return false;

  U64 pinnd = pinned();
  int ks = king_square();
  int ec = (whos_move() == WHITE ? BLACK : WHITE);
  int eks = king_square(ec);
  int frm = (move & 0x3f);
  int to = ((move & 0xfc0) >> 6);
  int mt = ((move & 0xf000) >> 12);
  int piece = piece_on(frm);

  // sanity checks 
  if (to == eks) return false;
  if (color_on(frm) != whos_move() || piece == PIECE_NONE) return false;
  if (color_on(to) == whos_move()) return false;
  if ((mt == QUIET || is_quiet_promotion(move) || mt == EP) && color_on(to) != COLOR_NONE) return false;
  if ((mt == CAPTURE || is_cap_promotion(move)) && color_on(to) != ec) return false;
  if (mt <= PROMOTION && piece != PAWN) return false;
  if (mt > PROMOTION && mt <= PROMOTION_CAP && piece != PAWN) return false;

  if (piece == KNIGHT)
    {
      U64 tmp = (SquareBB[to] & PseudoAttacksBB(KNIGHT, frm));
      if (!tmp)
	{
	  return false;
	}
    }

  if (piece == KING)
    {
      if (mt == CASTLE_KS || mt == CASTLE_QS) return true; // assumed legal
      U64 tmp = (SquareBB[to] & PseudoAttacksBB(KING, frm));
      if (!tmp)
	{
	  return false;
	}
    }

  if (piece == PAWN)
    {
      int cdiff = (COL(frm) - COL(to));
      if (cdiff == -1 || cdiff == 1)
	{
	  if ((mt == CAPTURE || mt == PROMOTION_CAP)
	      && mt != EP && color_on(to) == COLOR_NONE)
	    {
	      return false;
	    }
	}
      else if (cdiff == 0 && mt == QUIET)
	{
	  if (color_on(to) != COLOR_NONE)
	    {
	      return false;
	    }
	}
    }

  U64 mask = all_pieces();
  if (piece == BISHOP)
    {
      U64 bmvs = attacks<BISHOP>(mask, frm);
      U64 tmp = (SquareBB[to] & bmvs);
      if (!tmp)
	{
	  return false;
	}
    }
  if (piece == ROOK)
    {
      U64 rmvs = attacks<ROOK>(mask, frm);
      U64 tmp = (SquareBB[to] & rmvs);
      if (!tmp)
	{
	  return false;
	}
    }
  if (piece == QUEEN)
    {
      U64 qmvs = attacks<BISHOP>(mask, frm) | attacks<ROOK>(mask, frm);
      U64 tmp = (SquareBB[to] & qmvs);
      if (!tmp)
	{
	  return false;
	}
    }

  if (mt == EP || frm == ks || SquareBB[frm] & pinnd)
    {
      if (mt == EP && !legal_ep(frm, to, ks, ec)) return false;
      else if (frm == ks && !is_legal_km(ks, to, ec)) return false;
      else if ((SquareBB[frm] & pinnd) && !aligned(ks, frm, to)) return false;
    }

  // catch quiet moves which block/capture checking piece
  if (in_check() && frm != ks)
    {
      U64 frm_to = (SquareBB[frm] | SquareBB[to]); U64 t = SquareBB[to];
      U64 m = all_pieces() ^ frm_to;

      // ep already handled
      if (mt == CAPTURE || mt == PROMOTION_CAP) { pieces[ec][piece_on(to)] ^= t; m |= t; }
      bool incheck = is_attacked(ks, ec ^ 1, ec, m);
      if (mt == CAPTURE || mt == PROMOTION_CAP)  pieces[ec][piece_on(to)] ^= t;

      return !incheck;
    }
  return true;
}
int Board::phase()
{
  int tot_cnt = 0;
  for (int pt = 1; pt < PIECES - 1; ++pt) {
    tot_cnt += number_of_arr[WHITE][pt] + number_of_arr[BLACK][pt];
  }
  return (tot_cnt >= 6 ? MIDDLE_GAME : END_GAME);
}
// nb: assumes move is legal
bool Board::gives_check(U16& move)
{
  U64 m = all_pieces() ^ (SquareBB[get_from(move)] | SquareBB[get_to(move)]);
  return is_attacked(king_square(position->stm ^ 1), position->stm ^ 1, position->stm, m);
}

bool Board::legal_ep(int frm, int to, int ks, int ec)
{
  int csq = to + (ec == WHITE ? NORTH : SOUTH);
  U64 m = (all_pieces() ^ SquareBB[frm] ^ SquareBB[csq]) | SquareBB[to];
  return (!(attacks<BISHOP>(m, ks) & (get_pieces(ec, QUEEN) | get_pieces(ec, BISHOP)))
	  && !(attacks<ROOK>(m, ks) & (get_pieces(ec, QUEEN) | get_pieces(ec, ROOK))));
}
bool Board::is_legal_km(const U8 &ks, const U8 &to, const U8 &ec)
{
  U64 m = (all_pieces() ^ SquareBB[ks]);
  return !is_attacked(to, position->stm, position->stm^1, m);
}
void Board::unset_castle_rights(U16 cr)
{
  if (cr == ALL_W) position->crights &= (0xC);
  if (cr == ALL_B) position->crights &= (0x3);
  if (cr != ALL_W && cr != ALL_B) position->crights ^= U16(cr);
}
bool Board::is_dangerous(U16& m, int piece)
{
  int ks = (whos_move() == BLACK ? *(sq_of<KING>(WHITE) + 1) : *(sq_of<KING>(BLACK) + 1));
  int to = get_to(m);
  if (piece == PAWN || piece == KNIGHT) return true;
  U64 tmp = position->checkers;
  if (more_than_one(tmp)) return true;
  else if (piece == ROOK && (row_distance(to, ks) <= 1 && col_distance(to, ks) <= 1)) return true;
  else if (piece == QUEEN && (row_distance(to, ks) <= 1 && col_distance(to, ks) <= 1)) return true;
  else if (piece == BISHOP && (row_distance(to, ks) <= 1 && col_distance(to, ks) <= 1)) return true;
  return false;
}
bool Board::is_draw(int sz)
{
  if (position->move50 > 99 || (!position->in_check && sz <= 0))
    return true;

  return is_repition_draw();
}
bool Board::is_repition_draw()
{
  BoardData* past = position; int count = 0;
  for (int i = 0; i < position->hmvs; i += 2)
    {
      if (past->previous && past->previous->previous) past = past->previous->previous;
      else return false;
      if (past->pKey == position->pKey)
	{
	  ++count;
	}
      if (count >= 1)
	{
	  return true;
	}
    }
  return false;
}
void Board::clear()
{
  memset(this, 0, sizeof(Board));
  for (int s = A1; s <= H8; s++)
    {
      color_on_arr[s] = COLOR_NONE;
      piece_on_arr[s] = PIECE_NONE;
    }

  for (int p = 0; p < PIECES - 1; ++p)
    {
      piece_diff[p] = 0;
    }

  for (int c = WHITE; c <= BLACK; c++)
    {
      for (int p = PAWN; p <= KING; p++)
	{
	  for (int j = 0; j < 10; j++) square_of_arr[c][p][j] = SQUARE_NONE;
	  for (int j = 0; j < SQUARES; j++) piece_index[c][p][j] = 0;
	  number_of_arr[c][p] = 0;
	}
    }
  start.stm = COLOR_NONE;
  start.eps = SQUARE_NONE;
  start.in_check = false;
  start.ks = SQUARE_NONE;
  start.king_square[WHITE] = SQUARE_NONE;
  start.king_square[BLACK] = SQUARE_NONE;
  start.captured = PIECE_NONE;
  start.crights = 0;
  start.move50 = 0;
  start.hmvs = 0;
  start.dKey = 0ULL;
  start.pKey = 0ULL;
  start.mKey = 0ULL;
  start.pawnKey = 0ULL;
  start.checkers = 0ULL;
  start.pinned[WHITE] = 0ULL;
  start.pinned[BLACK] = 0ULL;
  start.disc_checkers[WHITE] = 0ULL;
  start.disc_checkers[BLACK] = 0ULL;
  start.blockers[WHITE] = 0ULL;
  start.blockers[BLACK] = 0ULL;
  start.previous = 0;
  nodes_searched = qsnodes = msnodes = 0;
  castled[WHITE] = castled[BLACK] = false;
  position = &start;
}
// note : perf testing on linux reveals this method takes approx ~13% of execution time! (try to avoid using)
U64 Board::attackers_of(int s) 
{
  U64 mask = all_pieces();
  U64 battck = attacks<BISHOP>(mask, s);
  U64 rattck = attacks<ROOK>(mask, s);
  U64 qattck = battck | rattck;
  return ((PawnAttacksBB[BLACK][s] & pieces[WHITE][PAWN]) |
	  (PawnAttacksBB[WHITE][s] & pieces[BLACK][PAWN]) |
	  (PseudoAttacksBB(KNIGHT, s) & (pieces[BLACK][KNIGHT] | pieces[WHITE][KNIGHT])) |
	  (PseudoAttacksBB(KING, s) & (pieces[BLACK][KING] | pieces[WHITE][KING])) |
	  (battck & (pieces[WHITE][BISHOP] | pieces[BLACK][BISHOP])) |
	  (rattck & (pieces[WHITE][ROOK] | pieces[BLACK][ROOK])) |
	  (qattck & (pieces[WHITE][QUEEN] | pieces[BLACK][QUEEN])));
}
U64 Board::attackers_of(int s, U64& mask)
{
  U64 battck = attacks<BISHOP>(mask, s);
  U64 rattck = attacks<ROOK>(mask, s);
  U64 qattck = battck | rattck;
  return ((PawnAttacksBB[BLACK][s] & pieces[WHITE][PAWN]) |
	  (PawnAttacksBB[WHITE][s] & pieces[BLACK][PAWN]) |
	  (PseudoAttacksBB(KNIGHT, s) & (pieces[BLACK][KNIGHT] | pieces[WHITE][KNIGHT])) |
	  (PseudoAttacksBB(KING, s) & (pieces[BLACK][KING] | pieces[WHITE][KING])) |
	  (battck & (pieces[WHITE][BISHOP] | pieces[BLACK][BISHOP])) |
	  (rattck & (pieces[WHITE][ROOK] | pieces[BLACK][ROOK])) |
	  (qattck & (pieces[WHITE][QUEEN] | pieces[BLACK][QUEEN])));
}

bool Board::is_attacked(const U8 &ks, const U8 &fc, const U8 &ec, const U64& m) {
  U64 stepper_checks = ((PawnAttacksBB[fc][ks] & pieces[ec][PAWN]) |
			(PseudoAttacksBB(KNIGHT, ks) & pieces[ec][KNIGHT]) |
			(PseudoAttacksBB(KING, ks) & pieces[ec][KING]));
  if (stepper_checks != 0ULL) return true;
  
  return ((attacks<BISHOP>(m, ks) & (get_pieces(ec, QUEEN) | get_pieces(ec, BISHOP))) ||
	  (attacks<ROOK>(m, ks) & (get_pieces(ec, QUEEN) | get_pieces(ec, ROOK))));
}

bool Board::is_attacked(const U8 &ks, const U8 &fc, const U8 &ec) {
  U64 stepper_checks = ((PawnAttacksBB[fc][ks] & pieces[ec][PAWN]) |
			(PseudoAttacksBB(KNIGHT, ks) & pieces[ec][KNIGHT]) |
			(PseudoAttacksBB(KING, ks) & pieces[ec][KING]));
  if (stepper_checks != 0ULL) return true;

  U64 m = all_pieces();
  return (attacks<BISHOP>(m, ks) & (get_pieces(ec, QUEEN) | get_pieces(ec, BISHOP))) ||
    (attacks<ROOK>(m, ks) & (get_pieces(ec, QUEEN) | get_pieces(ec, ROOK)));
}
int Board::smallest_attacker(int sq, int color, int& from)
{
  U64 occupied = SquareBB[sq] & pieces_by_color[whos_move() == WHITE ? BLACK : WHITE];
  U64 attackers = attackers_of(sq) & pieces_by_color[color];
  int minv = 20; // largest value of any piece
  while (attackers && occupied && piece_on(sq) != KING)
    {
      int sq = pop_lsb(attackers);
      int pv = (int)piece_on(sq);
      if (pv < PIECES && minv > pv) { minv = pv; from = sq; }
    }
  if (minv == 20) minv = -1;
  return minv;
}
U64 Board::xray_attackers(const int& s, const int& c)
{
  U64 tmp = all_pieces(); 
  U64 attackers = 0ULL;
  while(true)
    {
      U64 a = attackers_of(s, tmp) & tmp;
      if (a)
	{
	  tmp ^= a;
	  attackers |= a;
	}
      else break;
    }
  return attackers & colored_pieces(c);
}

struct SeePiece
{
  Piece p;
  U16 v;
};

struct {
  bool operator()(const SeePiece& p1, const SeePiece& p2)
  {
    return p1.v < p2.v; 
  }
} LessThan;

int Board::see_move(const U16& move) {
  int to = get_to(move); int widx = 0; int bidx = 0;
  U64 tmp = all_pieces();
  U64 attackers = 0ULL;
  U64 white_bb = colored_pieces(WHITE); 
  U64 black_bb = colored_pieces(BLACK);
  SeePiece black_list[16];
  SeePiece white_list[16]; // final storage   

  while(true) {
    // note : we could have more than 10 pieces attacking the square but this is extremely unlikely
    SeePiece black[10]; SeePiece white[10]; 
    int white_sz = 0; int black_sz = 0;
    U64 a = attackers_of(to, tmp) & tmp;
    if (a) {
      tmp ^= a;
      attackers |= a;
    }
    else break;
    U64 white_attackers = a & white_bb;
    if (white_attackers) {
      while(white_attackers) {
	white[white_sz].p = piece_on(pop_lsb(white_attackers));
	white[white_sz].v = material.material_value(white[white_sz].p, MIDDLE_GAME);
	white_sz++;
      }
    }
    U64 black_attackers = a & black_bb;
    if (black_attackers) {
      while(black_attackers) {
	black[black_sz].p = piece_on(pop_lsb(black_attackers));
	black[black_sz].v = material.material_value(black[black_sz].p, MIDDLE_GAME);
	black_sz++;
      }	  
    }
    std::sort(white, white + white_sz, LessThan); 
    std::sort(black, black + black_sz, LessThan);

    // manual copy the sorted list into the final sorted array
    for(int i=0; i<white_sz; ++i, ++widx) white_list[widx] = white[i]; 
    for(int i=0; i<black_sz; ++i, ++bidx) black_list[bidx] = black[i]; 
  }
  int to_piece = piece_on(to); int color = whos_move() ^ 1; int w = 0; int b = 0;
  int score = ((to_piece != PIECE_NONE) ? material.material_value(to_piece, MIDDLE_GAME) : 0);
  
  for(int i=0; i < 2*MIN(widx, bidx)-1; ++i) {
    int victim = (color == WHITE ? black_list[b++].p : white_list[w++].p);
    color ^= 1;
    int d = material.material_value(victim, MIDDLE_GAME);
    score += (i%2 == 0 ? -d : d);
  }
  return score;
}

int Board::see_sign(U16& move)
{
  if (get_movetype(move) == CAPTURE &&
      material.material_value(piece_on(get_from(move)), phase()) <=
      material.material_value(piece_on(get_to(move)), phase())) {
    
    return material.material_value(piece_on(get_to(move)), phase()) -
      material.material_value(piece_on(get_from(move)), phase());
  }
  return see_move(move);
}

// note: standard method to return a score based on the "most reasonable"
// capturing order at a given square.  Negative values imply the capture is losing, positive imply 
// winning (for side on the move).
// todo: pins + checks + handle king captures .. promotion captures??
int Board::see(int to)
{
  int value = 0;
  int from = -1;

  // sets the from square of the smallest attacking piece
  int piece = smallest_attacker(to, whos_move(), from);

  // return if no more attacking pieces
  if (piece >= 0)
    {
      U16 m = MOVE_NONE;
      int val = 0;
      m = (from | (to << 6) | (CAPTURE << 12));
      val = material.material_value(piece_on(to), phase());

      BoardData pd;
      do_move(pd, m);
      value = val - see(to);
      undo_move(m);
    }
  return value;
}
// return if we are in check or not (updated from do/undo move)
bool Board::in_check()
{
  return position->in_check;
}
bool Board::can_castle(const U16& cr)
{
  if (!(position->crights & cr)) return false;
  switch (cr)
    {
    case (W_KS) : if (is_attacked(F1, WHITE, BLACK) || is_attacked(G1, WHITE, BLACK) || piece_on(H1) != ROOK) return false; break;
    case (W_QS) : if (is_attacked(C1, WHITE, BLACK) || is_attacked(D1, WHITE, BLACK) || piece_on(A1) != ROOK) return false; break;
    case (B_KS) : if (is_attacked(F8, BLACK, WHITE) || is_attacked(G8, BLACK, WHITE) || piece_on(H8) != ROOK) return false; break;
    case (B_QS) : if (is_attacked(C8, BLACK, WHITE) || is_attacked(D8, BLACK, WHITE) || piece_on(A8) != ROOK) return false; break;
    default: return true; break;
    }
  return true;
}
void Board::print()
{
  for (int r = ROW8; r >= ROW1; r--)
    {
      printf("   +---+---+---+---+---+---+---+---+\n");
      printf(" %d ", r + 1);
      for (int c = COL1; c <= COL8; c++)
	{
	  int s = 8 * r + c;
	  bool occupied = false;

	  for (int p = PAWN; p <= KING; p++)
	    {
	      if (U64(s) & pieces[WHITE][p])
		{
		  std::cout << "|" << "(" << SanPiece[p] << ")";
		  occupied = true;
		}
	      if (U64(s) & pieces[BLACK][p])
		{
		  std::cout << "| " << SanPiece[p + 6] << " ";
		  occupied = true;
		}
	    }
	  if (!occupied) std::cout << "|   ";
	}
      std::cout << "|\n";
    }
  printf("   +---+---+---+---+---+---+---+---+\n");
  printf("     a   b   c   d   e   f   g   h\n");
}
U64 Board::pinned_to(int s)
{
  int fc = position->stm;
  int ec = (fc == WHITE ? BLACK : WHITE);
  U64 pinned = 0ULL;
  if (piece_on_arr[s] == PIECE_NONE) return pinned;
  U64 sliders = ((pieces[ec][BISHOP] & PseudoAttacksBB(BISHOP, s)) |
		 (pieces[ec][ROOK] & PseudoAttacksBB(ROOK, s)) |
		 (pieces[ec][QUEEN] & PseudoAttacksBB(QUEEN, s)));
  if (!sliders) return pinned;
  do
    {
      int sq = pop_lsb(sliders);
      U64 tmp = (BetweenBB[sq][s] & all_pieces()) ^ (SquareBB[s] | SquareBB[sq]);
      if (!more_than_one(tmp)) pinned |= tmp;
    } while (sliders);
  return pinned & colored_pieces(fc);
}
U64 Board::pinned()
{
  int fc = position->stm;
  int ec = (fc ^ 1);
  int ks = position->ks;
  U64 pinned = 0ULL;
  U64 sliders = ((pieces[ec][BISHOP] & PseudoAttacksBB(BISHOP, ks)) |
		 (pieces[ec][ROOK] & PseudoAttacksBB(ROOK, ks)) |
		 (pieces[ec][QUEEN] & PseudoAttacksBB(QUEEN, ks)));
  if (!sliders) return pinned;
  do
    {
      int sq = pop_lsb(sliders);
      U64 tmp = (BetweenBB[sq][ks] & all_pieces()) ^ (SquareBB[ks] | SquareBB[sq]);
      if (!more_than_one(tmp)) pinned |= tmp;
    } while (sliders);
  return pinned & colored_pieces(fc);
}
U64 Board::discovered_checkers(int c)
{
  return position->disc_checkers[c];
}
// note - calls from qsearch when move has *NOT* been made yet
// so consider move to be a move *AGAINST* the enemy king 
bool Board::dangerous_check(U16& move, bool discoveredBlocker)
{
  // almost always worth searching a discovered check...
  if (discoveredBlocker) return true;

  int us = whos_move();
  int them = us == WHITE ? BLACK : WHITE;
  int eks = square_of_arr[them][KING][1];
  int from = get_from(move);
  int to = get_to(move);
  int p = piece_on(from);
  U64 back_rank = (them == BLACK ? RowBB[ROW8] : RowBB[ROW1]);
  bool back_rank_check = (SquareBB[to] & back_rank);

  // flag back rank checks as always dangerous
  if (back_rank_check && p >= ROOK) return true;

  // more than one check is usually dangerous
  U64 checks = checkers(); bool multiple_checks = (more_than_one(checks));
  if (multiple_checks) return true;


  U64 king_region = PseudoAttacksBB(KING, eks);
  bool is_contact = (king_region & SquareBB[to]);

  if (is_contact && (p >= ROOK || p == PAWN)) return true;

  bool is_defended = attackers_of(to) & colored_pieces(them);
  if (!is_defended && (p == PAWN || p == KNIGHT)) return true;

  bool is_discovered = position->disc_checkers[us] & SquareBB[from];
  return is_discovered;
}
// nb. this method computes the candidates from scratch
void Board::compute_discovered_candidates(int c)
{
  position->disc_checkers[c] = 0ULL;
  position->blockers[c] = 0ULL;
  int them = c == WHITE ? BLACK : WHITE;
  int eks = square_of_arr[them][KING][1];
  U64 rcandidates = KingVisionBB[them][ROOK][eks] & (get_pieces(c, ROOK) | get_pieces(c, QUEEN));

  U64 bcandidates = KingVisionBB[them][BISHOP][eks] & (get_pieces(c, BISHOP) | get_pieces(c, QUEEN));
  U64 candidates = bcandidates | rcandidates;
  if (candidates)
    {
      while (candidates)
	{
	  int f = pop_lsb(candidates);
	  U64 CandidateMask = (BetweenBB[eks][f] & colored_pieces(c)) ^ (SquareBB[f]);
	  U64 tmp = CandidateMask; // local copy
	  if (tmp && count(tmp) == 1)
	    {
	      position->disc_checkers[c] |= SquareBB[f];
	      position->blockers[c] |= CandidateMask;
	    }
	}
    }
}
U64 Board::discovered_blockers(int c)
{
  return position->blockers[c];
}
U64 Board::pinned(int c, int * pinners) {
  int fc = c;
  int ec = fc ^ 1;
  int ks = square_of_arr[c][KING][1];
  U64 pinned = 0ULL;
  U64 sliders = ((pieces[ec][BISHOP] & PseudoAttacksBB(BISHOP, ks)) |
		 (pieces[ec][ROOK] & PseudoAttacksBB(ROOK, ks)) |
		 (pieces[ec][QUEEN] & PseudoAttacksBB(QUEEN, ks)));
  if (!sliders) return pinned;
  do
    {
      int sq = pop_lsb(sliders);
      U64 tmp = (BetweenBB[sq][ks] & all_pieces()) ^ (SquareBB[ks] | SquareBB[sq]);
      if (!more_than_one(tmp)) { pinned |= tmp; pinners[pop_lsb(tmp)] = piece_on(sq); } 
    } while (sliders);
  return pinned & colored_pieces(fc);
}

bool Board::pawn_on_7(int c) {
  U64 row = RowBB[(c == BLACK ? 1 : 6)];
  return row & pieces[c][PAWN];
}

void Board::set_piece(char& c, int s) {
  for (int p = W_PAWN; p <= B_KING; ++p)
    {
      if (c == SanPiece[p])
	{
	  Color color = color_of(p);

	  // adjust colored piece to bare piece type (pawn, knight, etc..)
	  int piece = (p > W_KING ? p - 6 : p);

	  // update the colored piece arrays
	  pieces[color][piece] |= SquareBB[s];
	  pieces_by_color[color] |= SquareBB[s];
	  color_on_arr[s] = color;

	  // update the piece counter/index arrays
	  number_of_arr[color][piece]++;
	  piece_index[color][piece][s] = number_of_arr[color][piece];

	  // update zobrist keys and square info for this piece 
	  square_of_arr[color][piece][number_of_arr[color][piece]] = s;
	  piece_on_arr[s] = piece;
	  position->pKey ^= Zobrist::piece_rands(s, color, piece);

	  if (p == W_KING || p == B_KING)
	    {
	      king_sq_bm[color] |= SquareBB[s];
	    }
	  else
	    {
	      position->mKey ^= Zobrist::piece_rands(s, color, piece);
	      (color == WHITE ? piece_diff[piece]++ : piece_diff[piece]--);
	    }
	  if (p == W_PAWN || p == B_PAWN) position->pawnKey ^= Zobrist::piece_rands(s, color, piece);

	}
    }
}
void Board::move_piece(int c, int p, int idx, int frm, int to)
{
  square_of_arr[c][p][idx] = to;
  piece_on_arr[to] = p;
  piece_on_arr[frm] = PIECE_NONE;
  piece_index[c][p][frm] = 0;
  piece_index[c][p][to] = idx;
  color_on_arr[to] = c;
  color_on_arr[frm] = COLOR_NONE;
  // zobrist keys
  position->pKey ^= (Zobrist::piece_rands(frm, c, p) | Zobrist::piece_rands(to, c, p));

  if (p == PAWN)
    {
      position->pawnKey ^= (Zobrist::piece_rands(frm, c, p) | Zobrist::piece_rands(to, c, p));
    }
}
void Board::remove_piece(int c, int p, int s)
{
  int tmp_idx = piece_index[c][p][s];
  int max_idx = number_of_arr[c][p];
  int tmp_sq = square_of_arr[c][p][max_idx];
  square_of_arr[c][p][tmp_idx] = square_of_arr[c][p][max_idx];
  square_of_arr[c][p][max_idx] = SQUARE_NONE;
  piece_index[c][p][tmp_sq] = tmp_idx;
  number_of_arr[c][p]--;
  piece_index[c][p][s] = 0;
  piece_on_arr[s] = PIECE_NONE;
  color_on_arr[s] = COLOR_NONE;
  (c == WHITE ? piece_diff[p]-- : piece_diff[p]++);
  // zobrist keys
  position->pKey ^= Zobrist::piece_rands(s, c, p);
  position->mKey ^= Zobrist::piece_rands(s, c, p);
  if (p == PAWN)  position->pawnKey ^= Zobrist::piece_rands(s, c, p);
}
void Board::add_piece(int c, int p, int s)
{
  number_of_arr[c][p]++;

  square_of_arr[c][p][number_of_arr[c][p]] = s;
  piece_on_arr[s] = p;
  piece_index[c][p][s] = number_of_arr[c][p];

  (c == WHITE ? piece_diff[p]++ : piece_diff[p]--);
  color_on_arr[s] = c;
  // zobrist keys
  position->pKey ^= Zobrist::piece_rands(s, c, p);
  position->mKey ^= Zobrist::piece_rands(s, c, p);
  if (p == PAWN)  position->pawnKey ^= Zobrist::piece_rands(s, c, p);
}
