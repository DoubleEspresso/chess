#include "globals.h"
#include "utils.h"
#include "bits.h"

#include <cmath>

namespace Globals {

  U64 RowBB[ROWS];
  U64 ColBB[COLS];
  U64 ColorBB[COLORS];               // colored squares (white or black)
  U64 NeighborColsBB[COLS];          // returns the neighbor columns (for isolated pawns)
  U64 NeighborRowsBB[ROWS];          // returns the neighbor rows (for king safety/bank rank mate)
  U64 PassedPawnBB[COLORS][SQUARES]; // in front of and each side of the pawn
  U64 BetweenBB[SQUARES][SQUARES];   // bitmap of squares aligned between 2 squares, used for pins
  U64 SquareBB[SQUARES + 2];             // store each square as a U64 for fast access
  U64 SmallCenterBB;
  U64 BigCenterBB;
  U64 CornersBB;
  U64 LongDiagonalsBB;
  U64 BoardEdgesBB;
  U64 PawnMaskLeft[2];
  U64 PawnMaskRight[2];
  U64 PawnMaskAll[2];

  // evaluation/utility bitmaps
  U64 PawnAttacksBB[COLORS][SQUARES];          // the pawn attacks (by color)
  U64 AttacksBB[PIECES - 1][SQUARES];            // knight,bishop,rook,queen,king
  U64 BishopMask[SQUARES];                     // bishop mask (outer board edges are trimmed)
  U64 RookMask[SQUARES];                       // rook mask (outer board edges are trimmed)
  U64 QueenMask[SQUARES];                      // queen mask (outer board edges are trimmed)
  U64 CastleMask[2][2];                        // determine castle state of each side
  U64 KingSafetyBB[COLORS][SQUARES];           // generic array around king (for pawn shelter computation)
  U64 KingVisionBB[COLORS][PIECES - 1][SQUARES]; // pseudo attack bitboards centered at each king-square
  U64 SpaceBehindBB[COLORS][SQUARES];          // all space behind a square
  U64 SpaceInFrontBB[COLORS][SQUARES];         // all space in front of a square
  U64 ColoredSquaresBB[COLORS];					 // squares of a specific color
  U64 CenterMaskBB;
  U64 kSidePawnStormBB;
  U64 qSidePawnStormBB;
  int SearchReductions[2][2][MAXDEPTH][MAXDEPTH]; // search reduction table (same as in stockfish)

  // the main initialization routine, which loads all the 
  // bitboard data listed in the globals namespace, these 
  // data are used to compute move hashes for the bishop/rook 
  // as well as aid the evaluation/searching methods
  bool init() {
    // fill the square bitboard - a lookup table for
    // converting each square into a U64 data type
    for (int s = 0; s < SQUARES + 2; ++s) SquareBB[s] = U64(s);

    // init the colored squares bitboard - a lookup table
    // useful for the bishop evaluations/color weaknesses
    ColoredSquaresBB[WHITE] = 0ULL;
    ColoredSquaresBB[BLACK] = 0ULL;

    // fill search reduction array
    // index assignment [pv_node][improving][depth][move count]
    for (int sd = 0; sd < MAXDEPTH; ++sd) {
      for (int mc = 0; mc < MAXDEPTH; ++mc) {
        // pv nodes
        double pvr = log(double(sd + 1)) * log(double(mc + 1)) / 3.0;
        double non_pvr = 0.25 + log(double(sd + 1)) * log(double(mc + 1)) / 2.0;

        SearchReductions[1][0][sd][mc] = int(pvr >= 1.0 ? pvr + 0.5 : 0);
        SearchReductions[1][1][sd][mc] = int(non_pvr >= 1.0 ? non_pvr + 0.5 : 0);

        // non-pv nodes
        SearchReductions[0][0][sd][mc] = SearchReductions[1][0][sd][mc];
        SearchReductions[0][1][sd][mc] = SearchReductions[0][1][sd][mc];

        if (SearchReductions[0][0][sd][mc] > 1)
          SearchReductions[0][0][sd][mc] += 1;

        //printf("..sd[pv][improving][%d][%d]=%d\n",sd, mc, SearchReductions[1][1][sd][mc]);
      }
    }

    // the center mask (smaller center - just 4 squares)
    CenterMaskBB = (SquareBB[D4] | SquareBB[D5] | SquareBB[E4] | SquareBB[E5]);
    // middle 16 squares
    BigCenterBB = CenterMaskBB |
      (SquareBB[C4] | SquareBB[C5] |
       SquareBB[F4] | SquareBB[F5] |
       SquareBB[E3] | SquareBB[E6] |
       SquareBB[D3] | SquareBB[D6] |
       SquareBB[C3] | SquareBB[C6] |
       SquareBB[F3] | SquareBB[F6]);
    
    // pawn storm detection on k/q side
    kSidePawnStormBB = SquareBB[F6] | SquareBB[F5] | SquareBB[F4] | SquareBB[F3] |
      SquareBB[G6] | SquareBB[G5] | SquareBB[G4] | SquareBB[G3] |
      SquareBB[H6] | SquareBB[H5] | SquareBB[H4] | SquareBB[H3];
    qSidePawnStormBB = SquareBB[C6] | SquareBB[C5] | SquareBB[C4] | SquareBB[C3] |
      SquareBB[B6] | SquareBB[B5] | SquareBB[B4] | SquareBB[B3] |
      SquareBB[A6] | SquareBB[A5] | SquareBB[A4] | SquareBB[A3];

    // fill the rows/cols bitboards
    for (int r = 0; r < 8; ++r) {
        U64 row = 0ULL; U64 col = 0ULL;

        for (int c = 0; c < 8; ++c) {
          col |= SquareBB[c * 8 + r];
          row |= SquareBB[r * 8 + c];
        }
        RowBB[r] = row;
        ColBB[r] = col;
    }

    // pawn masks for captures
    for (int col = WHITE; col <= BLACK; ++col) {      
      PawnMaskLeft[col] = 0ULL;
      PawnMaskRight[col] = 0ULL;    
      PawnMaskAll[col] = 0ULL;
      int rmax = (col == WHITE ? ROW6 : ROW7);
      int rmin = (col == WHITE ? ROW2 : ROW3);
      for (int r = rmin; r <= rmax; ++r) { 
        for (int c = COL1; c <= COL7; ++c) PawnMaskLeft[col] |= SquareBB[r*8+c];
        for (int c = COL2; c <= COL8; ++c) PawnMaskRight[col] |= SquareBB[r*8+c];
        for (int c = COL1; c <= COL8; ++c) PawnMaskAll[col] |= SquareBB[r*8+c];
      }
    }

    // helpful definitions for board corners/edges
    BoardEdgesBB = RowBB[ROW1] | ColBB[COL1] | RowBB[ROW8] | ColBB[COL8];
    CornersBB = SquareBB[A1] | SquareBB[H1] | SquareBB[H8] | SquareBB[A8];

    // attack step data
    int pawn_deltas[2][2] = {{ 9,7 }, { -7,-9 }}; // one entry for each color
    int knight_deltas[8] = { -17, -15, -10, -6, 6, 10, 15, 17 };
    int king_deltas[8] = { 8, -8, -1, 1, 9, 7, -9, -7 };
    int bishop_deltas[4] = { 9, -7, 7, -9 };
    int rook_deltas[4] = { 8, -8, 1, -1 };
    int k_safety[2][3] = {{ 16, 15, 17 }, { -16,-15,-17 }};

    // neighboring rows
    for (int r = 0; r < 8; ++r) NeighborRowsBB[r] = (r == 0 ? RowBB[r + 1] : r == 7 ? RowBB[r - 1] : RowBB[r - 1] | RowBB[r + 1]);

    // main loop over all board squares
    for (int s = A1; s <= H8; ++s) {
      // the colored squares bitboard, black squares
        // exist on even rows, and even squares, all other squares are white
        if ((ROW(s) % 2 == 0) && (s % 2 == 0)) ColoredSquaresBB[BLACK] |= SquareBB[s];
        else if ((ROW(s) % 2) != 0 && (s % 2 != 0)) ColoredSquaresBB[BLACK] |= SquareBB[s];
        else ColoredSquaresBB[WHITE] |= SquareBB[s];

        // load the pawn attack bitmaps here
        for (int c = WHITE; c <= BLACK; c++)
          {
            PawnAttacksBB[c][s] = 0ULL;
            for (int step = 0; step < 2; step++)
              {
                int to = s + pawn_deltas[c][step];

                if (on_board(to) && row_distance(s, to) < 2 && col_distance(s, to) < 2)
                  {
                    PawnAttacksBB[c][s] |= SquareBB[to];
                  }
              }
          }


        // load the knight and king attack bitmaps here
        U64 knight_attacks = 0ULL;
        U64 king_attacks = 0ULL;
        for (unsigned step = 0; step < 8; ++step)
          {
            // the knight
            int to = s + knight_deltas[step];
            if (on_board(to) && row_distance(s, to) <= 2 && col_distance(s, to) <= 2)
              {
                knight_attacks |= SquareBB[to];
              }

            // the king
            to = s + king_deltas[step];
            if (on_board(to) && row_distance(s, to) <= 1 && col_distance(s, to) <= 1)
              {
                king_attacks |= SquareBB[to];
              }
          } // end loop over knight/king steps

        if (knight_attacks) AttacksBB[0][s] = knight_attacks;
        if (king_attacks) AttacksBB[4][s] = king_attacks;


        // load the bishop attacks here
        U64 bishop_attacks = 0ULL;

        for (unsigned step = 0; step < 4; ++step)
          {
            int j = 1;
            while (true)
              {
                int to = s + (j++) * bishop_deltas[step];

                if (on_board(to) && on_diagonal(s, to))
                  {
                    bishop_attacks |= SquareBB[to];
                  }
                else break;
              }
          }
        if (bishop_attacks)
          {
            AttacksBB[1][s] = bishop_attacks;

            // the trimmed bishop mask
            U64 tmp = bishop_attacks & BoardEdgesBB;
            BishopMask[s] = bishop_attacks ^ tmp;
          }


        // load the rook attacks here
        U64 rook_attacks = 0ULL;
        for (unsigned step = 0; step < 4; ++step)
          {
            int j = 1;
            while (true)
              {
                int to = s + (j++) * rook_deltas[step];
                if (on_board(to) && (same_row(s, to) || same_col(s, to)))
                  {
                    rook_attacks |= SquareBB[to];
                  }
                else break;
              }
          }

        // remove the outer bits for the rook masks
        if (rook_attacks)
          {
            U64 col_maxmin = (SquareBB[COL(s)]) | (SquareBB[(56 + COL(s))]);
            U64 row_maxmin = (SquareBB[8 * ROW(s)]) | (SquareBB[(8 * ROW(s) + 7)]);

            U64 m = col_maxmin | row_maxmin;

            RookMask[s] = rook_attacks ^ m;
            if (RookMask[s] & SquareBB[s]) RookMask[s] = RookMask[s] ^ SquareBB[s];

            AttacksBB[2][s] = rook_attacks;
          }

        // the queen attacks
        AttacksBB[3][s] = AttacksBB[1][s] | AttacksBB[2][s];

        // the queen masks
        QueenMask[s] = RookMask[s] | BishopMask[s];


        // Positional arrays : fill the evaluation/safety/utility arrays

        // 1. the passed pawn masks and isolated pawn masks in one shot
        // region of interest for passed pawns excludes the back ranks for either color
        U64 roi = ~(RowBB[COL1] | RowBB[COL8]);

        if (SquareBB[s] & roi)
          {
            // overwrite neighbor cols multiple times here, but should be fine
            NeighborColsBB[COL(s)] = 0ULL;

            for (int color = WHITE; color <= BLACK; ++color)
              {
                PassedPawnBB[color][s] = 0ULL;
                U64 neighbor_squares = (AttacksBB[4][s] & RowBB[ROW(s)]) | SquareBB[s];

                while (neighbor_squares)
                  {
                    int square = pop_lsb(neighbor_squares);
                    PassedPawnBB[color][s] |= infrontof_square(ColBB[COL(square)], color, square);

                    // collect just the neighbor columns for a given square
                    if (s != square) NeighborColsBB[COL(s)] |= ColBB[COL(square)];
                  }

              }
          }

        // 2. king safety bitboards, indexed by color
        // they are generic regions around king, to count pawn cover
        for (int color = WHITE; color <= BLACK; ++color)
          {
            KingSafetyBB[color][s] = AttacksBB[4][s];

            for (unsigned step = 0; step < 3; ++step)
              {
                int to = s + k_safety[color][step];

                if (on_board(to) && row_distance(s, to) <= 2 && col_distance(s, to) <= 2) KingSafetyBB[color][s] |= SquareBB[to];
              }
          }

        // 3. king vision bitboards, indexed by square
        // these arrays track all possible attacks to the king sitting at a given square
        for (int color = WHITE; color <= BLACK; ++color)
          {
            KingVisionBB[color][PAWN][s] = PawnAttacksBB[color][s]; KingVisionBB[color][KNIGHT][s] = AttacksBB[0][s];
            KingVisionBB[color][BISHOP][s] = AttacksBB[1][s]; KingVisionBB[color][ROOK][s] = AttacksBB[2][s];
            KingVisionBB[color][QUEEN][s] = AttacksBB[3][s];
          }

        // 4. the between bitboard, used for determining pins
        for (int s2 = A1; s2 <= H8; ++s2)
          {
            if (aligned(s, s2) && s != s2)
              {
                U64 btwn = 0ULL;

                int iter = 0;
                int delta = 0;

                int sq;

                if (col_distance(s, s2) == 0) delta = (s < s2 ? 8 : -8);
                else if (row_distance(s, s2) == 0) delta = (s < s2 ? 1 : -1);
                else if (on_diagonal(s, s2))
                  {
                    if (s < s2 && COL(s) < COL(s2)) delta = 9;
                    else if (s < s2 && COL(s) > COL(s2)) delta = 7;
                    else if (s > s2 && COL(s) < COL(s2)) delta = -7;
                    else delta = -9;
                  }

                if (delta != 0)
                  {
                    do
                      {
                        sq = s + iter * delta;
                        btwn |= SquareBB[sq];
                        iter++;
                      } while (sq != s2);
                  }
                BetweenBB[s][s2] = btwn;
              }
          }


        // 5. the space behind and in front of a given square (exluding the row of the square)
        for (int c = WHITE; c <= BLACK; ++c)
          {
            SpaceInFrontBB[c][s] = (c == WHITE ? BetweenBB[s][A8 + COL(s)] : BetweenBB[s][COL(s)]) ^ SquareBB[s];
            SpaceBehindBB[c][s] = (c == WHITE ? BetweenBB[s][COL(s)] : BetweenBB[s][A8 + COL(s)]) ^ SquareBB[s];
          }


      } // end loop over board squares

    // 6. the castle masks
    CastleMask[WHITE][0] = (U64(B1) | U64(C1) | U64(D1)); // qside
    CastleMask[WHITE][1] = (U64(F1) | U64(G1)); // kside
    CastleMask[BLACK][0] = (U64(B8) | U64(C8) | U64(D8));
    CastleMask[BLACK][1] = (U64(F8) | U64(G8));

    // long diagonals
    LongDiagonalsBB = (BetweenBB[A1][H8] | BetweenBB[H1][A8] | BetweenBB[A2][G8] | 
                       BetweenBB[H2][B8] | BetweenBB[B1][H7] | BetweenBB[G1][A7]);

    return true;
  }

  U64 PseudoAttacksBB(int piece, int square)
  {
    return AttacksBB[piece - 1][square];
  }
}
