#pragma once
#ifndef HEADWIG_BOARD_H
#define HEADWIG_BOARD_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstring>

#include "definitions.h"
#include "utils.h"

class Board;
class Thread;

// POD types to describe
// the details of the chessboard at a given
// instant in the search/eval/do/undo move routines
struct BoardData
{
	int stm;
	int eps;
	bool in_check;
	int ks;
	Piece captured;
	int move50;
	int hmvs;
	U16 crights;
	U64 mKey;
	U64 pKey;
	U64 dKey;
	U64 pawnKey;


	int king_square[2];
	U64 pinned[2];
	U64 checkers;
	BoardData* previous;
};

// the essential POD types to describe
// the details of the chessboard at a given
// instant in the search/eval

struct BoardDataReduced
{
	int stm;
	int eps;
	bool in_check;
	int ks;
	Piece captured;
	int move50;
	int hmvs;
	U16 crights;
	U64 mKey;
	U64 pKey;
	U64 dKey;
	U64 pawnKey;
};

class Board
{
public:
	Board();
	Board(std::istringstream& is);
	Board(Board& b);
	//Board(std::string fen, Board& b);
	Board(const Board& pos, Thread* th) { *this = pos; thread = th; }
	~Board();

	// position setup/related
	void from_fen(std::istringstream& is);
	//void to_fen();
	//void from_moves(std::vector<std::string> mvs);
	void clear();
	void set_piece(char& c, int s);
	void print();

	// return the game phase (middle or end game)
	int phase();

	// do/undo move actions/helper functions
	void do_move(BoardData& d, U16 m);
	void undo_move(U16 m);
	void do_null_move(BoardData& d);
	void undo_null_move();
	inline void move_piece(int c, int p, int idx, int frm, int to);
	inline void remove_piece(int c, int p, int s);
	inline void add_piece(int c, int p, int s);

	// sequential exchange evaluation during search (quality of captures)
	int see(int to);
	int see_move(U16& move);
	int smallest_attacker(int sq, int enemy_color, int& from); // returns ordered list from least to greatest value.

	// search data access
	inline void set_nodes_searched(int nodes);
	inline int get_nodes_searched();
	inline void inc_nodes_searched(int nodes);
	inline bool has_position();
	inline int whos_move();

	// piece access
	template<Piece> inline int * sq_of(int c);
	inline int * piece_deltas();
	int * piece_counts(int c);
	U64 non_pawn_material(int c);
	inline U64 get_pieces(int c, int p);
	inline U64 colored_pieces(int c);
	inline U64 all_pieces();
	int color_on(int s);
	Piece piece_on(int s);
	inline int get_ep_sq();

	// hash table keys for a given position
	inline U64 material_key() const;
	inline U64 pawn_key();
	inline U64 pos_key();
	inline U64 data_key();
	
	// king safety/evaluation related
	bool is_dangerous(U16& m, int piece);
	inline bool compute_in_check();
	bool in_check();
	bool gives_check(U16& move);
	U64 pinned();
	U64 pinned(int c);	
	inline int king_square(); // returns king square of color on the move
	inline int king_square(int c); // returns king square of either color
	U64 attackers_of(int s, U64& mask);
	U64 attackers_of(int s);

	// castling related
	bool can_castle(U16 cr);
	bool is_castled(Color c);
	void unset_castle_rights(U16 cr);
	U16 get_castle_rights(int c);

	// game state/mate draw
	//bool is_mated();
	bool is_mate();
	bool is_draw(int sz);

	// move properties access/types
	bool checks_king(U16& move);
	inline U64 checkers();
	bool is_quiet(U16& m);
	bool is_legal(U16& move);
	bool legal_ep(int frm, int to, int ks, int ec);
	bool is_legal_km(int ks, int to, int ec);

	// YBW thread access to board in threaded search
	Thread * get_worker() { return thread; }
	void set_worker(Thread * t) { thread = t; }

private:
	BoardData start;
	BoardData * position;
	Thread * thread;

	U64 pieces_by_color[COLORS];
	U64 pieces[COLORS][PIECES];
	U64 king_sq_bm[COLORS];

	int color_on_arr[SQUARES];
	int square_of_arr[COLORS][PIECES][11];
	int piece_on_arr[SQUARES];
	int number_of_arr[COLORS][PIECES];
	int piece_index[COLORS][PIECES][SQUARES];
	int piece_diff[PIECES - 1]; // no kings
	int nodes_searched;

	bool castled[COLORS];
};
inline U64 Board::non_pawn_material(int c)
{
	return pieces_by_color[c] ^ pieces[c][PAWN];
}

inline int Board::color_on(int s)
{
	return color_on_arr[s];
}

inline bool Board::is_castled(Color c)
{
	return castled[c];
}

inline void Board::inc_nodes_searched(int nodes)
{
	nodes_searched += nodes;
}

inline void Board::set_nodes_searched(int nodes)
{
	nodes_searched = nodes;
}

inline int Board::get_nodes_searched()
{
	return nodes_searched;
}

inline int * Board::piece_counts(int c)
{
	return number_of_arr[c];
}

inline U64 Board::pawn_key()
{
	return position->pawnKey;
}

inline U64 Board::pos_key()
{
	return position->pKey;
}

inline U64 Board::data_key()
{
	return position->dKey;
}

inline U64 Board::material_key() const
{
	return position->mKey;
}

inline int * Board::piece_deltas()
{
	return piece_diff;
}

inline int Board::whos_move()
{
	return position->stm;
}

inline U64 Board::get_pieces(int c, int p)
{
	return pieces[c][p];
}

inline U64 Board::colored_pieces(int c)
{
	return pieces_by_color[c];
}

inline int Board::king_square()
{
	return position->ks;
}

inline int Board::king_square(int c)
{
	return position->king_square[c];
}

inline U64 Board::checkers()
{
	return position->checkers;
}

inline int Board::get_ep_sq()
{
	return position->eps;
}

inline Piece Board::piece_on(int s)
{
	return Piece(piece_on_arr[s]);
}

template<Piece p>
inline int * Board::sq_of(int c)
{
	return square_of_arr[c][p];
}

inline U64 Board::all_pieces()
{
	return (pieces_by_color[WHITE] | pieces_by_color[BLACK]);
}

inline U16 Board::get_castle_rights(int c)
{
	return (c == WHITE ? (position->crights & 0x3) : (position->crights & 0xC));
}

inline bool Board::has_position()
{
	return position != 0;
}

extern Board ROOT_BOARD;
extern int ROOT_DEPTH;

#endif
