#pragma once

// the global definitions/enums for squares/colors
// pieces etc.

#ifndef HEDWIG_DEFINITIONS_H
#define HEDWIG_DEFINITIONS_H


#include <iostream>
#include <type_traits>
#include <cmath>

// build information definitions
#ifdef _WIN32
#define OS "Windows"
#define VERSION "v.2.0"
#endif

#define ENGINE_NAME "Hedwig"
#define AUTHOR "M.Glatzmaier"


#ifdef _WIN32
#define if1(x) if(x)
#define if0(x) if(x)
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop) )
#else
#define if1(x) if(__builtin_expect(!!(x),1))
#define if0(x) if(__builtin_expect(!!(x),0))
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

// maximum legal moves in any chess position
#define MAX_MOVES 246
#define MAX_THREADS 128


// score definitions
#define INF 9999
#define NINF -9999
#define VAL_NONE 10000
#define MATE_VAL 9998
#define MATED_VAL -9998
#define DRAW 0
#define MAXDEPTH 32
#define MAXPLY 64
#define MATE_IN_MAXPLY MATE_VAL-MAXPLY
#define MATED_IN_MAXPLY MATED_VAL+MAXPLY

// chess definitions
const std::string START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

enum Color { WHITE, BLACK, COLORS, COLOR_NONE };

enum Piece { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, PIECES, PIECE_NONE };

enum ColoredPiece
  {
    W_PAWN, W_KNIGHT, W_BISHOP,
    W_ROOK, W_QUEEN, W_KING,

    B_PAWN, B_KNIGHT, B_BISHOP,
    B_ROOK, B_QUEEN, B_KING,
    MAX_PIECES, COLORED_PIECE_NONE
  };

enum MoveType
  {
    MOVE_NONE, PROMOTION = 4, PROMOTION_CAP = 8, QUIET = 11, CAPTURE, EP, EVASION,
    CHECK, CASTLE, CASTLE_KS = 9, CASTLE_QS = 10, PSEUDO_LEGAL = 17, LEGAL, CAPTURE_CHECKS
  };

enum MaterialValue
{
	PawnValueMG = 150,
	KnightValueMG = 455,//365, 
    BishopValueMG = 490,// 380,
    RookValueMG = 655, 
    QueenValueMG = 1280,

    PawnValueEG = 170,
    KnightValueEG = 475,
    BishopValueEG = 510,
    RookValueEG = 685,
    QueenValueEG = 1335
  };

enum Square
  {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    SQUARES, SQUARE_NONE
  };

enum Castles
  {
    W_KS = 1, W_QS = 2, B_KS = 4, B_QS = 8, ALL_W = 9, ALL_B = 10, CASTLES_NONE = 0
  };
enum GamePhase { MIDDLE_GAME, END_GAME };

const std::string SanCols = "abcdefgh";

const std::string SanPiece = "PNBRQKpnbrqk";
const std::string SanSquares[64] =
  {
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
  };

enum Rows
  {
    ROW1, ROW2, ROW3, ROW4,
    ROW5, ROW6, ROW7, ROW8,
    ROWS, ROW_NONE
  };

enum Cols
  {
    COL1, COL2, COL3, COL4,
    COL5, COL6, COL7, COL8,
    COLS, COL_NONE
  };

enum Bound
  {
    BOUND_LOW, BOUND_EXACT, BOUND_HIGH, BOUND_NONE
  };

enum Direction
  {
    // king steps
    NORTH = 8, SOUTH = -8,
    EAST = 1, WEST = -1,
    NW = 8 - 1, NE = 8 + 1,
    SW = -8 - 1, SE = -8 + 1,

    // pawn steps
    NN = 16, SS = -16,
    EE = 2, WW = -2,

    // knight steps
    NNW = NN - 1, NNE = NN + 1,
    SSW = SS - 1, SSE = SS + 1,
    EEN = EE + NORTH, EES = EE + SOUTH,
    WWN = WW + NORTH, WWS = WW + SOUTH
  };

enum BenchType
  {
    PAWN_POS, KNIGHT_POS, BISHOP_POS, ROOK_POS, QUEEN_POS,
    MINORS, MAJORS, PERFT, ENDGAME, TTABLE, BENCH, DIVIDE
  };

inline float rounded(float number)
{
	return number < 0.0 ? ceil(number - 0.5) : floor(number + 0.5);
}
#endif
