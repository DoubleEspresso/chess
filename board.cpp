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

/*
  namespace
  {
  int compare(const void*a, const void*b) { return *(int*)a - *(int*)b; }
  }
*/
Board::Board() : position(0), thread(0)
{

}

Board::Board(std::istringstream& is) : position(0), thread(0)
{
  from_fen(is);
}

Board::Board(Board& b) : position(0), thread(0)
{
  memcpy(this, &b, sizeof(Board));
}

//Board::Board(ThreadWorker * t) : position(0), thread(0), w_pieces(0ULL), b_pieces(0ULL)
//{
//	//memcpy(this,&b,sizeof(Board)); 
//	this->thread = t;
//}

Board::~Board() { }

/////////////////////////////////////////////////
//  UCI specific routines - c'tor for new board
/////////////////////////////////////////////////
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

  // is the king in check?
  position->in_check = compute_in_check();

  // if in check, set the checkers bitmap
  (position->in_check ? position->checkers = (attackers_of(position->ks) & colored_pieces(position->stm == WHITE ? BLACK : WHITE)) : 0ULL);
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
  // copy the position state back to previous
  position = position->previous;
}

void Board::do_move(BoardData& d, U16 m)
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

  if0(from == position->ks)
    {
      position->ks = to;
      (us == WHITE ? position->crights &= 0xC : position->crights &= 0x3);
      position->king_square[us] = to;
    }
  if0(p == ROOK && (SquareBB[from] & CornersBB))
    {
      if (from == H1) { unset_castle_rights(W_KS); position->dKey ^= Zobrist::castle_rands(WHITE, W_KS); }
      if (from == A1) { unset_castle_rights(W_QS); position->dKey ^= Zobrist::castle_rands(WHITE, W_QS); }
      if (from == H8) { unset_castle_rights(B_KS); position->dKey ^= Zobrist::castle_rands(BLACK, B_KS); }
      if (from == A8) { unset_castle_rights(B_QS); position->dKey ^= Zobrist::castle_rands(BLACK, B_QS); }
    }
  position->captured = PIECE_NONE;
  if1(type != EP && type != PROMOTION && type != CASTLE)
    {
      if (type == QUIET)
	{
	  move_piece(us, p, idx, from, to);
	  pieces[us][p] ^= tfbit;
	  pieces_by_color[us] ^= tfbit;
	}
      if (type == CAPTURE)
	{
	  position->captured = Piece(piece_on_arr[to]);
	  remove_piece(them, position->captured, to);
	  move_piece(us, p, idx, from, to);
	  pieces[us][p] ^= tfbit;
	  pieces_by_color[us] ^= tfbit;
	  pieces[them][position->captured] ^= tbit;
	  pieces_by_color[us==WHITE?BLACK:WHITE] ^= tbit;
	}
    }
  // ep moves here
  if0(type == EP)
    {
      int tmp = to + (us == WHITE ? SOUTH : NORTH);
      position->captured = PAWN;
      remove_piece(them, PAWN, tmp);
      move_piece(us, PAWN, idx, from, to);

      pieces[them][PAWN] ^= SquareBB[tmp];
      pieces_by_color[us==WHITE?BLACK:WHITE] ^= SquareBB[tmp];
      pieces[us][p] ^= tfbit;
      pieces_by_color[us] ^= tfbit;
    }
  // normal promotion moves (no captures)
  if0(type != MOVE_NONE && type <= PROMOTION)
    {
      Piece pp = Piece(type);
      remove_piece(us, PAWN, from);
      add_piece(us, pp, to);
      pieces[us][p] ^= fbit;
      pieces[us][type] |= tbit;
      pieces_by_color[us] ^= tfbit;
    }
  // capture promotion moves 
  if0(type > PROMOTION && type <= PROMOTION_CAP)
    {
      position->captured = Piece(piece_on_arr[to]);
      Piece pp = Piece(type - 4);

      pieces[them][piece_on_arr[to]] ^= tbit;
      pieces_by_color[us==WHITE?BLACK:WHITE] ^= tbit;
      pieces[us][p] ^= fbit;
      pieces[us][type - 4] |= tbit;
      pieces_by_color[us] ^= tfbit;

      remove_piece(us, PAWN, from);
      remove_piece(them, position->captured, to);
      add_piece(us, pp, to);
    }
  // castle moves
  if0(type == CASTLE_KS || type == CASTLE_QS)
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
  position->checkers = attackers_of(position->ks) & colored_pieces(position->stm == WHITE ? BLACK : WHITE);
  position->in_check = position->checkers;
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
  if1(type != EP && type != PROMOTION && type != CASTLE)
    {
      if (type == QUIET)
	{
	  move_piece(us, p, idx, from, to);
	  pieces[us][p] ^= tfbit;
	  pieces_by_color[us] ^= tfbit;
	}

      if (type == CAPTURE)
	{
	  Piece cp = position->captured;
	  move_piece(us, p, idx, from, to);
	  add_piece(them, cp, from);
	  pieces[us][p] ^= tfbit;
	  pieces_by_color[us == WHITE ? BLACK : WHITE] |= fbit;
	  pieces[them][cp] |= fbit;
	  pieces_by_color[us] ^= tfbit;
	}
    }
  if0(type == EP)
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
  if0(type != MOVE_NONE && type <= PROMOTION)
    {
      Piece pp = Piece(piece_on_arr[from]);
      remove_piece(us, pp, from);
      add_piece(us, PAWN, to);
      pieces[us][pp] ^= fbit;
      pieces[us][PAWN] |= tbit;
      pieces_by_color[us] ^= tfbit;
    }
  // capture promotion moves 
  if0(type > PROMOTION && type <= PROMOTION_CAP)
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
  if0(type == CASTLE_KS || type == CASTLE_QS)
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
bool Board::is_legal(U16& move)
{
  U64 pinnd = pinned();	
  int ks = king_square();
  int ec = (whos_move() == WHITE ? BLACK : WHITE);
  int frm = (move & 0x3f);
  int to = ((move & 0xfc0) >> 6);
  int mt = ((move & 0xf000) >> 12);

  if (mt == EP || frm == ks || SquareBB[frm]&pinnd)
    {
      if (mt == EP && !legal_ep(frm, to, ks, ec)) return false;
      else if (frm == ks && !is_legal_km(ks, to, ec)) return false;
      else if ((SquareBB[frm]&pinnd) && !aligned(ks, frm, to)) return false;
    }

  return true;
}

//TODO incremental updates here.
int Board::phase()
{
  int tot_cnt = 0;
  for (int pt = 0; pt<PIECES - 1; ++pt)
    {
      if (pt > 0) tot_cnt += number_of_arr[WHITE][pt] + number_of_arr[BLACK][pt];
    }
  return (tot_cnt >= 6 ? MIDDLE_GAME : END_GAME);
}

// WARNING :: only call this after a "do-move" call!
bool Board::gives_check(U16& move)
{
  int to = get_to(move);
  //assert(to < 64);
  U64 to_bm = SquareBB[to];
  U64 attackers = attackers_of(king_square());
  return to_bm & attackers;
}

// general routine to check if move gives check - this is slow..
bool Board::checks_king(U16& move)
{
  bool incheck = false;
  BoardData pd;
  do_move(pd, move);
  incheck = compute_in_check();
  undo_move(move);
  return incheck;
}
bool Board::legal_ep(int frm, int to, int ks, int ec)
{
  int csq = to + (ec == WHITE ? NORTH : SOUTH);
  U64 m = (all_pieces() ^ SquareBB[frm] ^ SquareBB[csq]) | SquareBB[to];
  return (!(attacks<BISHOP>(m, ks) & (get_pieces(ec, QUEEN) | get_pieces(ec, BISHOP)))
	  && !(attacks<ROOK>(m, ks) & (get_pieces(ec, QUEEN) | get_pieces(ec, ROOK))));
}

bool Board::is_legal_km(int ks, int to, int ec)
{
  U64 tmp = (all_pieces() ^ SquareBB[ks]);
  return !(attackers_of(to, tmp) & colored_pieces(ec));
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
  if (position->move50 > 99 || (!position->in_check && !sz))
    return true;

  BoardData* state_data = position;
  for (int i = 2, e = position->move50; i <= e; i += 2)
    {
      if (state_data->previous && state_data->previous->previous) state_data = state_data->previous->previous;
      else return false;
      if (state_data->pKey == position->pKey) return true;
    }
  return false;
}

bool Board::is_mate()
{
  if (!position->in_check) return false;
  // we are in check, do we have legal moves?
  MoveGenerator mvs(*this);
  return !mvs.size();
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
	  for (int j = 0; j<10; j++) square_of_arr[c][p][j] = SQUARE_NONE;
	  for (int j = 0; j<SQUARES; j++) piece_index[c][p][j] = 0;
	  number_of_arr[c][p] = 0;
	}
    }
  start.stm = COLOR_NONE;
  start.eps = SQUARE_NONE;
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
  nodes_searched = 0;
  castled[WHITE] = castled[BLACK] = false;
  position = &start;
}
U64 Board::attackers_of(int s)
{
  U64 mask = all_pieces();
  U64 battck = attacks<BISHOP>(mask, s);
  U64 rattck = attacks<ROOK>(mask, s);
  U64 qattck = battck | rattck;
  return ((PawnAttacksBB[BLACK][s] & pieces[WHITE][PAWN]) |
	  (PawnAttacksBB[WHITE][s] & pieces[BLACK][PAWN]) |
	  (PseudoAttacksBB(KNIGHT,s) & pieces[BLACK][KNIGHT]) |
	  (PseudoAttacksBB(KNIGHT, s) & pieces[WHITE][KNIGHT]) |
	  (PseudoAttacksBB(KING, s) & pieces[WHITE][KING]) |
	  (PseudoAttacksBB(KING, s) & pieces[BLACK][KING]) |
	  (battck & pieces[WHITE][BISHOP]) |
	  (battck & pieces[BLACK][BISHOP]) |
	  (rattck & pieces[WHITE][ROOK]) |
	  (rattck & pieces[BLACK][ROOK]) |
	  (qattck & pieces[WHITE][QUEEN]) |
	  (qattck & pieces[BLACK][QUEEN]));
}
U64 Board::attackers_of(int s, U64& mask)
{
  U64 battck = attacks<BISHOP>(mask, s);
  U64 rattck = attacks<ROOK>(mask, s);
  U64 qattck = battck | rattck;
  return ((PawnAttacksBB[BLACK][s] & pieces[WHITE][PAWN]) |
	  (PawnAttacksBB[WHITE][s] & pieces[BLACK][PAWN]) |
	  (PseudoAttacksBB(KNIGHT, s) & pieces[BLACK][KNIGHT]) |
	  (PseudoAttacksBB(KNIGHT, s) & pieces[WHITE][KNIGHT]) |
	  (PseudoAttacksBB(KING, s) & pieces[WHITE][KING]) |
	  (PseudoAttacksBB(KING, s) & pieces[BLACK][KING]) |
	  (battck & pieces[WHITE][BISHOP]) |
	  (battck & pieces[BLACK][BISHOP]) |
	  (rattck & pieces[WHITE][ROOK]) |
	  (rattck & pieces[BLACK][ROOK]) |
	  (qattck & pieces[WHITE][QUEEN]) |
	  (qattck & pieces[BLACK][QUEEN]));
}

int Board::smallest_attacker(int sq, int color, int& from)
{

  U64 occupied = SquareBB[sq] & pieces_by_color[whos_move() ==WHITE?BLACK:WHITE];
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

// see_move will compute see() on any type of move, quiet or capture
// to determine the safety of the "to-square".
int Board::see_move(U16& move)
{

  int square = get_to(move);
  int value = material.material_value(piece_on(square), phase());
  
  BoardData pd;
  do_move(pd, move);
  value += (-see(square));
  undo_move(move);
  
  return value;
}

// note: standard method to return a score based on the "most reasonable"
// capturing order at a given square.  Negative values imply the capture is losing, positive imply 
// winning.
int Board::see(int to)
{
  int value = 0;
  int from = -1;

  // sets the from square of the smallest attacking piece
  int piece = smallest_attacker(to, whos_move(), from);
  //std::cout << " smallest " << (whos_move() == WHITE ? "white" : "black") << " attacker of " << SanSquares[to] << " is " << SanPiece[piece] << std::endl;

  // return if no more attacking pieces
  if (piece >= 0)
    {
      // build move
      U16 m = MOVE_NONE;
      int val = 0;

      m = (from | (to << 6) | (CAPTURE << 12));
      val = material.material_value(piece_on(to), phase());

      BoardData pd;
      do_move(pd, m);

      value = val - see(to);
      //std::cout << " see mv " << UCI::move_to_string(m) << " value " << value << std::endl;

      undo_move(m);
    }
  return value;
}

// given a position, compute if a piece is attacking our
// king
inline bool Board::compute_in_check()
{
  int fc = position->stm;
  int ec = (fc == WHITE ? BLACK : WHITE);
  int ks = position->ks;
  return (attackers_of(ks) & colored_pieces(ec));
}

// return if we are in check or not (updated from do/undo move)
bool Board::in_check()
{
  return position->in_check;
}

bool Board::can_castle(U16 cr)
{
  U64 enemies = 0ULL;
  if (cr == W_KS || cr == W_QS) enemies |= colored_pieces(BLACK);
  else enemies |= colored_pieces(WHITE);

  if (!(position->crights & cr)) return false;
  switch (cr)
    {
    case (W_KS) : if ((attackers_of(F1)&enemies) || (attackers_of(G1)&enemies) || piece_on(H1) != ROOK) return false; break;
    case (W_QS) : if ((attackers_of(C1)&enemies) || (attackers_of(D1)&enemies) || piece_on(A1) != ROOK) return false; break;
    case (B_KS) : if ((attackers_of(F8)&enemies) || (attackers_of(G8)&enemies) || piece_on(H8) != ROOK) return false; break;
    case (B_QS) : if ((attackers_of(C8)&enemies) || (attackers_of(D8)&enemies) || piece_on(A8) != ROOK) return false; break;
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

U64 Board::pinned()
{
  int fc = position->stm;
  int ec = (fc == WHITE ? BLACK : WHITE);
  int ks = position->ks;
  U64 pinned = 0ULL;
  U64 sliders = ((pieces[ec][BISHOP] & PseudoAttacksBB(BISHOP,ks)) |
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

U64 Board::pinned(int c)
{
  int fc = c;
  int ec = (fc == WHITE ? BLACK : WHITE);
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

bool Board::pawn_on_7(int c)
{
  U64 row = RowBB[(c == BLACK ? 1 : 6)];
  return row & pieces[c][PAWN];
}

void Board::set_piece(char& c, int s)
{
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

	  // dbg printout
	  //printf("..(dbg) c(%d) s(%d) p(%d) z(%lu)\n",color,s,Piece(piece),zobrist.piece_rands(Square(s), color, Piece(piece)));

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
	
  // zobrist keys
  position->pKey ^= Zobrist::piece_rands(s, c, p);
  position->mKey ^= Zobrist::piece_rands(s, c, p);
  if (p == PAWN)  position->pawnKey ^= Zobrist::piece_rands(s, c, p);
}
