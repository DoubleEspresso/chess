#include "move.h"
#include "board.h"
#include "magic.h"
#include "uci.h"

using namespace Globals;
using namespace Magic;

inline U64 MoveGenerator::move_pawns(int c, int d, U64& b)
{
	return (c == WHITE ? b << d : b >> d);
}

// main move serialization functions -- these will accept 
// a bitboard and return a chopped set of encoded moves 
template<MoveType mt>
inline void MoveGenerator::serialize(U64& b, int from)
{
	while (b)
	{
		int to = pop_lsb(b);
		U16 m = (from | (to << 6) | (mt << 12));
		list[last++].m = m;
	}
}

template<MoveType mt>
inline void MoveGenerator::serialize(U64& b, Direction d)
{
	while (b)
	{
		int to = pop_lsb(b);
		int from = to + d;
		U16 m = (from | (to << 6) | (mt << 12));
		list[last++].m = m;
	}
}

template<MoveType mt>
inline void MoveGenerator::serialize_promotion(U64 &b, Direction d)
{
	while (b)
	{
		int to = pop_lsb(b);
		int from = to + d;
		for (int i = mt - 3; i <= mt; ++i)
		{
			U16 m = (from | (to << 6) | (i << 12));
			list[last++].m = m;
		}
	}
}

MoveList * MoveGenerator::generate_pawn_moves(Board &b, MoveType mt)
{
	int stm = b.whos_move();
	U64 empty = ~(b.all_pieces());
	U64 enemies = b.colored_pieces(stm == WHITE ? BLACK : WHITE);
	U64 pawns = b.get_pieces(stm, PAWN);
	int eps = b.get_ep_sq();
	U64 rank2 = (stm == WHITE ? RowBB[ROW2] : RowBB[ROW7]);
	U64 rank7 = (stm == WHITE ? RowBB[ROW7] : RowBB[ROW2]);
	U64 rank5 = (stm == WHITE ? RowBB[ROW5] : RowBB[ROW4]);
	const Direction back1 = (stm == WHITE ? SOUTH : NORTH);
	const Direction back2 = (stm == WHITE ? SS : NN);
	const Direction bleft = (stm == WHITE ? SW : NE);
	const Direction bright = (stm == WHITE ? SE : NW);
	U64 pawns7 = pawns & rank7;
	U64 pawnsNot7 = pawns ^ pawns7;
	U64 pawns5 = pawns & rank5;
	U64 pawns2 = pawns & rank2;

	// quite pawn pushes (not promotions)
	if1(mt == QUIET || mt == EVASION || mt == LEGAL || mt == PSEUDO_LEGAL)
	{
		U64 singlePushes = (stm == WHITE ? pawnsNot7 << NORTH : pawnsNot7 >> NORTH) & empty;
		if (singlePushes) serialize<QUIET>(singlePushes, back1);

		U64 tmp = move_pawns(stm, NORTH, pawns2) & empty;
		U64 doublePushes = move_pawns(stm, NORTH, tmp) & empty;
		if (doublePushes) serialize<QUIET>(doublePushes, back2);
	}

	// pawn captures (not promotions -- includes ep-moves)
	if (mt == CAPTURE || mt == LEGAL || mt == EVASION || mt == PSEUDO_LEGAL)
	{
		U64 not_on_right = pawnsNot7 ^ (pawnsNot7 & (stm == WHITE ? ColBB[COL8] : ColBB[COL1]));
		U64 not_on_left = pawnsNot7 ^ (pawnsNot7 & (stm == WHITE ? ColBB[COL1] : ColBB[COL8]));
		U64 right_caps = (stm == WHITE ? not_on_right << NE : not_on_right >> NE) & enemies;
		U64 left_caps = (stm == WHITE ? not_on_left << NW : not_on_left >> NW) & enemies;
		if (right_caps) serialize<CAPTURE>(right_caps, bleft);
		if (left_caps) serialize<CAPTURE>(left_caps, bright);

		// ep - captures
		U64 pawns4_no_right = pawns5 ^ (pawns5 & (stm == WHITE ? ColBB[COL8] : ColBB[COL1]));
		U64 pawns4_no_left = pawns5 ^ (pawns5 & (stm == WHITE ? ColBB[COL1] : ColBB[COL8]));
		U64 ep_right = (stm == WHITE ? pawns4_no_right << NE : pawns4_no_right >> NE) & SquareBB[eps]; // !! note danger (SQUARE_NONE = 65, extended SquareBB to account for this case).
		U64 ep_left = (stm == WHITE ? pawns4_no_left << NW : pawns4_no_left >> NW) & SquareBB[eps];
		if (ep_right) serialize<EP>(ep_right, bleft);
		if (ep_left) serialize<EP>(ep_left, bright);
	}

	// promotions - quiet
	if0(mt == QUIET || mt == LEGAL || mt == PROMOTION || (mt == EVASION && pawns7) || mt == PSEUDO_LEGAL)
	{
		U64 quitePromotions = (stm == WHITE ? pawns7 << NORTH : pawns7 >> NORTH) & empty;
		if (quitePromotions) serialize_promotion<PROMOTION>(quitePromotions, back1);
	}
	// promotions - captures
	if0(mt == CAPTURE || mt == LEGAL || mt == PROMOTION || (mt == EVASION && pawns7) || mt == PSEUDO_LEGAL)
	{
		U64 pawns7NoRightCol = pawns7 ^ (pawns7 & (stm == WHITE ? ColBB[COL8] : ColBB[COL1]));
		U64 pawns7NoLeftCol = pawns7 ^ (pawns7 & (stm == WHITE ? ColBB[COL1] : ColBB[COL8]));
		U64 caps7Right = (stm == WHITE ? pawns7NoRightCol << NE : pawns7NoRightCol >> NE) & enemies;
		U64 caps7Left = (stm == WHITE ? pawns7NoLeftCol << NW : pawns7NoLeftCol >> NW) & enemies;

		if (caps7Right) serialize_promotion<PROMOTION_CAP>(caps7Right, bleft);
		if (caps7Left) serialize_promotion<PROMOTION_CAP>(caps7Left, bright);
	}
	return list;
}

// piece captures
MoveList * MoveGenerator::generate_legal_caps(Board& b)
{
	int  stm = b.whos_move();
	U64  enemies = b.colored_pieces(stm == WHITE ? BLACK : WHITE);
	U64  mask = b.all_pieces();

	// the knights
	int *sqs = b.sq_of<KNIGHT>(stm);
	for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
	{
		U64 attackBB = PseudoAttacksBB(KNIGHT, from) & enemies;
		if (attackBB) serialize<CAPTURE>(attackBB, from);
	}
	// the king 
	int from = b.king_square();
	U64 attackBB = PseudoAttacksBB(KING, from) & enemies;
	if (attackBB) serialize<CAPTURE>(attackBB, from);

	// the bishop
	sqs = b.sq_of<BISHOP>(stm);
	for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
	{
		U64 bmvs = attacks<BISHOP>(mask, from);
		U64 attackBB = bmvs & enemies;
		if (attackBB) serialize<CAPTURE>(attackBB, from);
	}

	// the rook
	sqs = b.sq_of<ROOK>(stm);
	for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
	{
		U64 rmvs = attacks<ROOK>(mask, from);
		U64 attackBB = rmvs & enemies;
		if (attackBB) serialize<CAPTURE>(attackBB, from);
	}

	// the queen
	sqs = b.sq_of<QUEEN>(stm);
	for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
	{
		U64 qmvs = attacks<BISHOP>(mask, from) | attacks<ROOK>(mask, from);
		U64 attackBB = qmvs & enemies;
		if (attackBB) serialize<CAPTURE>(attackBB, from);
	}
	generate_pawn_moves(b, CAPTURE);

	return list;
}

// normal moves - quiets and captures
MoveList * MoveGenerator::generate_piece_moves(Board& b, MoveType mt)
{
	int  stm = b.whos_move();
	U64  enemies = b.colored_pieces(stm == WHITE ? BLACK : WHITE);
	U64  mask = b.all_pieces();
	U64  empty = ~mask;

	// the knights
	int *sqs = b.sq_of<KNIGHT>(stm);
	for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
	{
		if (mt == QUIET)
		{
			U64 quites = PseudoAttacksBB(KNIGHT, from) & empty;
			if (quites) serialize<QUIET>(quites, from);
		}
		else if (mt == CAPTURE)
		{
			U64 attackBB = PseudoAttacksBB(KNIGHT, from) & enemies;
			if (attackBB) serialize<CAPTURE>(attackBB, from);
		}
	}

	// the king 
	int from = b.king_square();
	if (mt == QUIET)
	{
		U64 quites = PseudoAttacksBB(KING, from) & empty;
		if (quites) serialize<QUIET>(quites, from);
	}
	else if (mt == CAPTURE)
	{
		U64 attackBB = PseudoAttacksBB(KING, from) & enemies;
		if (attackBB) serialize<CAPTURE>(attackBB, from);
	}

	// the bishop
	sqs = b.sq_of<BISHOP>(stm);
	for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
	{
		U64 bmvs = attacks<BISHOP>(mask, from);
		if (mt == QUIET)
		{
			U64 quites = bmvs & empty;
			if (quites) serialize<QUIET>(quites, from);
		}
		else if (mt == CAPTURE)
		{
			U64 attackBB = bmvs & enemies;
			if (attackBB) serialize<CAPTURE>(attackBB, from);
		}
	}

	// the rook
	sqs = b.sq_of<ROOK>(stm);
	for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
	{
		U64 rmvs = attacks<ROOK>(mask, from);
		if (mt == QUIET)
		{
			U64 quites = rmvs & empty;
			if (quites) serialize<QUIET>(quites, from);
		}
		else if (mt == CAPTURE)
		{
			U64 attackBB = rmvs & enemies;
			if (attackBB) serialize<CAPTURE>(attackBB, from);
		}
	}

	// the queen
	sqs = b.sq_of<QUEEN>(stm);
	for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
	{
		U64 qmvs = attacks<BISHOP>(mask, from) | attacks<ROOK>(mask, from);
		if (mt == QUIET)
		{
			U64 quites = qmvs & empty;
			if (quites) serialize<QUIET>(quites, from);
		}
		else if (mt == CAPTURE)
		{
			U64 attackBB = qmvs & enemies;
			if (attackBB) serialize<CAPTURE>(attackBB, from);
		}
	}
	return list;
}

MoveList * MoveGenerator::generate_piece_evasions(Board& board, MoveType type)
{
	// collect position data
	int  stm = board.whos_move();
	U64  enemies = board.colored_pieces(stm == WHITE ? BLACK : WHITE);
	U64  pawns = board.get_pieces(stm, PAWN);
	U64  mask = board.all_pieces();
	U64  empty = ~mask;
	int  ks = board.king_square();
	U64  target = 0ULL;
	U64  checkers = board.checkers();

	//king -- note king captures come last (they are usually dangerous/illegal)
	U64 kmvs = PseudoAttacksBB(KING, ks);
	if (type == QUIET)
	{
		U64 quites = kmvs & empty;
		if (quites) serialize<QUIET>(quites, ks);
	}
	// king capture (inserted at the end of the list on purpose)
	if (type == CAPTURE)
	{
		U64 attackBB = kmvs & enemies;
		if (attackBB) serialize<CAPTURE>(attackBB, ks);
	}

	// set the target bitmap for EVASIONS -- this is a bitmap between the king and the enemy checker
	// if more than one checker exists, only king moves will be legal.
	U64 tmp = checkers;
	U64 tmp2 = checkers;
	if (!more_than_one(tmp))
	{
		int check_sq = pop_lsb(tmp2);
		int checker = board.piece_on(check_sq);
		(checker == QUEEN || checker == BISHOP || checker == ROOK) ? target |= (BetweenBB[ks][check_sq] ^ SquareBB[check_sq] ^ SquareBB[ks]) : target |= 0ULL;
	}
	else return list;
	enemies = checkers;
	// pawns
	if1(pawns)
	{
		int eps = board.get_ep_sq();
		U64 rank2 = (stm == WHITE ? RowBB[ROW2] : RowBB[ROW7]);   // for double-pushes
		U64 rank7 = (stm == WHITE ? RowBB[ROW7] : RowBB[ROW2]);   // for promotions
		U64 rank5 = (stm == WHITE ? RowBB[ROW5] : RowBB[ROW4]);   // for ep captures
		const Direction back1 = (stm == WHITE ? SOUTH : NORTH);
		const Direction back2 = (stm == WHITE ? SS : NN);
		const Direction backleft = (stm == WHITE ? SW : NE);
		const Direction backright = (stm == WHITE ? SE : NW);
		U64   pawns7 = pawns & rank7;          // all pawns on 7th rank
		U64   pawnsNot7 = pawns ^ pawns7;         // all other pawns 
		U64   pawns5 = pawns & rank5;          // rank5 pawns for ep-moves
		U64   pawns2 = pawns & rank2;

		// pawn blocks (quiet moves)
		if (type == QUIET)
		{
			U64  singlePushes = move_pawns(stm, NORTH, pawnsNot7) & target;
			if (singlePushes) serialize<QUIET>(singlePushes, back1);
			U64  tmp = move_pawns(stm, NORTH, pawns2) &  empty;
			U64  doublePushes = move_pawns(stm, NORTH, tmp) & target;
			if (doublePushes) serialize<QUIET>(doublePushes, back2);
		}

		// pawn capture evasions
		if (type == CAPTURE)
		{
			U64 pawnsNoRightCol = pawnsNot7 ^ (pawnsNot7 & (stm == WHITE ? ColBB[COL8] : ColBB[COL1]));
			U64 pawnsNoLeftCol = pawnsNot7 ^ (pawnsNot7 & (stm == WHITE ? ColBB[COL1] : ColBB[COL8]));
			U64 capsRight = move_pawns(stm, NE, pawnsNoRightCol) & enemies;
			U64 capsLeft = move_pawns(stm, NW, pawnsNoLeftCol) & enemies;
			if (capsRight) serialize<CAPTURE>(capsRight, backleft);
			if (capsLeft) serialize<CAPTURE>(capsLeft, backright);
			// ep captures
			U64 pawns4NoRightCol = pawns5 ^ (pawns5 & (stm == WHITE ? ColBB[COL8] : ColBB[COL1]));
			U64 pawns4NoLeftCol = pawns5 ^ (pawns5 & (stm == WHITE ? ColBB[COL1] : ColBB[COL8]));
			U64 epCapRight = move_pawns(stm, NE, pawns4NoRightCol) & SquareBB[eps]; // !! note danger (SQUARE_NONE = 64, squareBB[0:64], carefully convert).
			U64 epCapLeft = move_pawns(stm, NW, pawns4NoLeftCol) & SquareBB[eps];
			if (epCapRight) serialize<EP>(epCapRight, backleft);
			if (epCapLeft) serialize<EP>(epCapLeft, backright);
		}

		// promotions -- quiet and capture
		if0(pawns7)
		{
			if (type == QUIET)
			{
				U64 quitePromotions = move_pawns(stm, NORTH, pawns7) & target;
				if (quitePromotions) serialize_promotion<PROMOTION>(quitePromotions, back1);
			}
			// promotions - captures
			if (type == CAPTURE)
			{
				U64 pawns7NoRightCol = pawns7 ^ (pawns7 & (stm == WHITE ? ColBB[COL8] : ColBB[COL1]));
				U64 pawns7NoLeftCol = pawns7 ^ (pawns7 & (stm == WHITE ? ColBB[COL1] : ColBB[COL8]));
				U64 caps7Right = move_pawns(stm, NE, pawns7NoRightCol) & enemies;
				U64 caps7Left = move_pawns(stm, NW, pawns7NoLeftCol) & enemies;
				if (caps7Right) serialize_promotion<PROMOTION_CAP>(caps7Right, backleft);
				if (caps7Left) serialize_promotion<PROMOTION_CAP>(caps7Left, backright);
			}
		}
	}
	//knight
	int *sqs = board.sq_of<KNIGHT>(stm);
	for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
	{
		if (type == QUIET)
		{
			U64 quites = PseudoAttacksBB(KNIGHT, from) & target;
			if (quites) serialize<QUIET>(quites, from);
		}
		if (type == CAPTURE)
		{
			U64 attackBB = PseudoAttacksBB(KNIGHT, from) & enemies;
			if (attackBB) serialize<CAPTURE>(attackBB, from);
		}
	}
	// bishop
	sqs = board.sq_of<BISHOP>(stm);
	for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
	{
		U64 bmvs = attacks<BISHOP>(mask, from);
		if (type == QUIET)
		{
			U64 quites = bmvs & target;
			if (quites) serialize<QUIET>(quites, from);
		}
		if (type == CAPTURE)
		{
			U64 attackBB = bmvs & enemies;
			if (attackBB) serialize<CAPTURE>(attackBB, from);
		}
	}
	// rook
	sqs = board.sq_of<ROOK>(stm);
	for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
	{
		U64 rmvs = attacks<ROOK>(mask, from);
		if (type == QUIET)
		{
			U64 quites = rmvs & target;
			if (quites) serialize<QUIET>(quites, from);
		}
		if (type == CAPTURE)
		{
			U64 attackBB = rmvs & enemies;
			if (attackBB) serialize<CAPTURE>(attackBB, from);
		}
	}
	// queen
	sqs = board.sq_of<QUEEN>(stm);
	for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
	{
		U64 qmvs = (attacks<BISHOP>(mask, from) | attacks<ROOK>(mask, from));
		if (type == QUIET)
		{
			U64 quites = qmvs & target;
			if (quites) serialize<QUIET>(quites, from);
		}
		if (type == CAPTURE)
		{
			U64 attackBB = qmvs & enemies;
			if (attackBB) serialize<CAPTURE>(attackBB, from);
		}
	}
	return list;
}

MoveList * MoveGenerator::generate_legal_castle_moves(Board& b)
{
	int stm = b.whos_move();
	U64 empty = ~(b.all_pieces());
	U16 cr = b.get_castle_rights(stm);
	U16 ks_c = stm == WHITE ? W_KS : B_KS;
	U16 qs_c = stm == WHITE ? W_QS : B_QS;

	if (!cr) return list;
	if ((stm == WHITE && b.king_square() != E1) || (stm == BLACK && b.king_square() != E8)) return list;
	if (cr & ks_c)
	{
		if (((empty & CastleMask[stm][1]) == CastleMask[stm][1]) && b.can_castle((stm == WHITE ? W_KS : B_KS)))
		{
			int frm = (stm == WHITE ? E1 : E8);
			int to = (stm == WHITE ? G1 : G8);
			U16 m = frm | (to << 6);
			append<CASTLE_KS>(m);
		}
	}
	if (cr & qs_c)
	{
		if ((empty & CastleMask[stm][0]) == CastleMask[stm][0] && b.can_castle((stm == WHITE ? W_QS : B_QS)))
		{
			int frm = (stm == WHITE ? E1 : E8);
			int to = (stm == WHITE ? C1 : C8);
			U16 m = frm | (to << 6);
			append<CASTLE_QS>(m);
		}
	}
	return list;
}

inline bool MoveGenerator::is_legal_ep(int from, int to, int ks, int ec, Board& b)
{
	int csq = to + (ec == WHITE ? NORTH : SOUTH);
	U64 m = (b.all_pieces() ^ SquareBB[from] ^ SquareBB[csq]) | SquareBB[to];
	return (!(attacks<BISHOP>(m, ks) & (b.get_pieces(ec, QUEEN) | b.get_pieces(ec, BISHOP)))
		&& !(attacks<ROOK>(m, ks) & (b.get_pieces(ec, QUEEN) | b.get_pieces(ec, ROOK))));
}

inline bool MoveGenerator::is_legal_km(int ks, int to, int ec, Board& b, int type)
{
	U64 tmp = 0ULL;
	tmp = (type == CAPTURE ? b.all_pieces() ^ SquareBB[ks] ^ SquareBB[to] : b.all_pieces() ^ SquareBB[ks]);
	return !(b.attackers_of(to, tmp) & b.colored_pieces(ec));
}

template<MoveType mt>
inline void MoveGenerator::append(U16 m)
{
	list[last++].m = (m | mt << 12);
}

MoveList* MoveGenerator::generate_pseudo_legal(Board& b, MoveType mt)
{
	if (b.in_check())
	{
		generate_piece_evasions(b, mt);
	}
	else
	{
		if (b.phase() == MIDDLE_GAME)
		{
			generate_piece_moves(b, mt);
			if (mt == QUIET) generate_legal_castle_moves(b);
			generate_pawn_moves(b, mt);
		}
		else
		{
			generate_pawn_moves(b, mt);
			generate_piece_moves(b, mt);
			if (mt == QUIET) generate_legal_castle_moves(b);
		}
	}

	// mark all moves as legal, they are checked one-by-one
	// during the main search
	for (int i = 0; i < last; ++i)
	{
		legal_i[i] = i;
	}
	return list;
}


MoveList* MoveGenerator::generate(Board& b, MoveType mt)
{
	if0(mt == CAPTURE)
	{
		if0(b.in_check())
		{
			generate_piece_evasions(b, mt);
		}
	  else generate_legal_caps(b);
	}
  else // usually reserved for LEGAL type
  {
	  if0(b.in_check())
	  {
		  generate_piece_evasions(b, QUIET);
		  generate_piece_evasions(b, CAPTURE);
	  }
	  else
	  {
		  // so it plays chess... :/
		  generate_pawn_moves(b, QUIET);
		  generate_pawn_moves(b, CAPTURE);
		  generate_piece_moves(b, QUIET);
		  generate_piece_moves(b, CAPTURE);
	  }
  }
  if0((mt == LEGAL || mt == CASTLE || mt == PSEUDO_LEGAL) &&
	  !b.in_check()) generate_legal_castle_moves(b);

  // mark all moves as legal, they are checked one-by-one
  // during the main search
  if0(mt == PSEUDO_LEGAL && last > 0)
  {
	  // allows accessing mvs.move() in move.h
	  for (int i = 0; i < last; ++i)
	  {
		  legal_i[i] = i;
	  }
	  return list;
  }

  unsigned int _sz = last;
  unsigned int iter = 0;
  if (mt != PSEUDO_LEGAL && _sz > 0)
  {
	  U64 pinned = b.pinned();
	  int ks = b.king_square();
	  int ec = (b.whos_move() == WHITE ? BLACK : WHITE);
	  for (unsigned int i = 0; i < _sz; ++i)
	  {
		  int frm = (list[i].m & 0x3f);
		  int to = ((list[i].m & 0xfc0) >> 6);
		  int mt = ((list[i].m & 0xf000) >> 12);
		  int legal = 1;
		  if (mt == EP || frm == ks || SquareBB[frm] & pinned)
		  {
			  if (mt == EP && !is_legal_ep(frm, to, ks, ec, b)) legal = 0;
			  else if (frm == ks && !is_legal_km(ks, to, ec, b, mt)) legal = 0;
			  else if ((SquareBB[frm] & pinned) && !aligned(ks, frm, to)) legal = 0;
		  }
		  (legal == 1 ? legal_i[iter++] = i : last--);
	  }
  }
  return list;
}

// note - removed checksKing parameter (not used currently)
MoveList* MoveGenerator::generate_qsearch_mvs(Board& b, MoveType mt, bool genChecks)
{
	if (b.in_check())
	{
		generate_piece_evasions(b, mt);
	}
	else
	{
	  generate_piece_moves(b, mt);
	  generate_pawn_moves(b, mt);	  
	}

	// mark all moves as legal
	for (int i = 0; i < last; ++i)
	{
		legal_i[i] = i;
	}

	if (b.in_check()) return list;

	// we are not in check, remove checks from the list
	unsigned int _sz = last;
	unsigned int iter = 0;
	if (_sz > 0)
	{
		for (unsigned int i = 0; i < _sz; ++i)
		{
			int legal = 1;
			//if (genChecks && (!b.gives_check(list[i].m) || !b.is_qsearch_mv(list[i].m))) legal = 0;
			//if (!genChecks && !b.is_qsearch_mv(list[i].m)) legal = 0;
			if (!b.is_qsearch_mv(list[i].m)) legal = 0;
			(legal == 1 ? legal_i[iter++] = i : last--);
		}
	}
	return list;
}

MoveList* MoveGenerator::generate_qsearch_caps_checks(Board& b)
{
	if0(b.in_check())
	{
		generate_piece_evasions(b, CAPTURE);
		generate_piece_evasions(b, QUIET);
	}
  else
  {
	  generate_pawn_moves(b, PSEUDO_LEGAL);
	  generate_piece_moves(b, PSEUDO_LEGAL);
  }
  unsigned int _sz = last;
  unsigned int iter = 0;

  if (_sz > 0)
  {
	  U64 pinned = b.pinned();
	  int ks = b.king_square();
	  int ec = (b.whos_move() == WHITE ? BLACK : WHITE);
	  for (unsigned int i = 0; i < _sz; ++i)
	  {
		  int frm = (list[i].m & 0x3f);
		  int to = ((list[i].m & 0xfc0) >> 6);
		  int type = ((list[i].m & 0xf000) >> 12);
		  int legal = 1;

		  if (type == EP && !is_legal_ep(frm, to, ks, ec, b)) legal = 0;
		  else if (frm == ks && !is_legal_km(ks, to, ec, b, type)) legal = 0;
		  else if ((SquareBB[frm] & pinned) && !aligned(ks, frm, to)) legal = 0;

		  //filters
		  if (!b.in_check())
		  {
			  if (legal == 1 && (type == CAPTURE || b.gives_check(list[i].m))) legal = 1;
			  else legal = 0;
		  }
		  (legal == 1 ? legal_i[iter++] = i : last--);
	  }
  }
  return list;
}
