#include "evaluate.h"
#include "definitions.h"
#include "squares.h"
#include "globals.h"
#include "utils.h"
#include "magic.h"
#include "material.h"
#include "pawns.h"

using namespace Globals;
using namespace Magic;


#include "evaluate.h"


int piece_vals_mg[5] = { PawnValueMG, KnightValueEG, BishopValueMG, RookValueMG, QueenValueMG };
int piece_vals_eg[5] = { PawnValueEG, KnightValueEG, BishopValueEG, RookValueEG, QueenValueEG };

// -- Bug Positions --
//
// na3 bm, will play ne5 and eval to 0 at depths > 10.  Seems sensitive to hash table size (larger the better).
// removing all eval routines (except material count) helps it find bm.
// position fen r1b1kb1r/2p1pppp/p1n2n2/1p1q4/2Np4/1P3N2/PBPPPPPP/R2QKB1R w KQkq b6 0 8 
//
// bishop should pin knight here, white's next move can easily lose control of the center, was playing NxN and
// losing a piece quite quickly afterward
// position fen r1bqkb1r/pp3ppp/2n2n2/2P1p3/3pN3/5N2/PPPBPPPP/R2QKB1R w KQkq - 4 9 
//
// na4 here obviously...but eventually gets stuck with ne2 instead (eval diff of ~1!)
// position fen 3q1rk1/3bb1p1/prp1p3/2p1Qp2/7p/PPNP3P/2P2PPB/1R3RK1 w - - 1 21
//
// white can save the knight + bishop here, was losing a piece in this position
// position fen 8/1p2kpp1/5n1p/1N1p4/1n2pNB1/4P3/3PKPPP/7r w - - 0 24 
//
// common qgd position (white takes pawn and loses game)
// rnbqkb1r/pp2pppp/8/2pp4/3PnB2/2N2N2/PPP1PPPP/R2QKB1R w KQkq - 2 5 
//
// white should not play qd3 (like in game) pawn forks --> lost piece are common bugs
// rnbqkb1r/pp2p1pp/5p2/2Pp4/5B2/2P2N2/P1P1PPPP/R2QKB1R w KQkq - 0 7 
//
// bd6 here as black? -- should always be avoided..
// r1bqkb1r/pppp1ppp/2n2n2/4p3/4P3/P1N2N2/1PPP1PPP/R1BQKB1R b KQkq e3 0 4 
//
// can white avoid mate for > 10 moves here?
// 8/pp2kp1p/1r2p1b1/8/r1pP1R2/P1P5/K1P1B1PP/4R3 w - - 21 29 
//
// previously played rc1 here and quickly mated...avoid mate in 10??
// 8/1p3p2/4p2p/1N1p2k1/1P1P2P1/P2r1p1P/2rb1P2/1R1R2K1 w - - 3 33 
//
// plays nb5 here, then qe6 wins immediately
// r3r1k1/pbp2ppp/3p1q2/2p5/3Nn3/2P1BQ2/PPP2PPP/2KR3R w - - 0 17
//
// white should evaluate a loss in this position (mate or material loss unavoidable..nxf3)
// 1r2r1k1/1Qpq1ppp/p2b1n2/8/3n4/2NP1B2/PP1B1PPP/R4RK1 w - - 1 18 
//
// white plays qe2 here and loses bishop (and game) immediately (only seems to happen in arena vs. other engines..)
// r1bqkb1r/ppp1nppp/8/4p3/4B3/5N2/PPPP1PPP/R1BQK2R w KQkq - 1 8
//
// white plays ne5 and loses piece ???
// rn1qkbnr/pp1bpppp/8/1Bp5/3pP3/2N2N2/PPPP1PPP/R1BQK2R w KQkq - 2 5 
// 
// white plays qc4 here...drops piece and loses
// 2r1kb1r/pp2n1pp/2q1N3/2p2p2/Q3p3/2P5/PP1P1PPP/R1B2RK1 w k - 1 16 
//
// white plays bc2 here and loses game .. currently takes 20sec to find g2g3 .. :-/ 
// r1bq1rk1/1p2bppp/p1n1p3/3n4/3p4/2PB1N2/PP1PQPPP/RNB2RK1 w - - 1 12 
//
// win game as white from here
// 6k1/2p2pp1/p1N4p/P3PP1P/2P5/6PK/r2bR3/8 w - - 1 46 
//
// as black, find better .. played a5 ... ended up losing knight in game after a few moves, stockfish prefers OOO
// r3kb1r/2p3p1/ppnq3p/3pp3/4P2P/2PB1P2/P1PB1P2/RQ2K2R b KQkq - 1 15 
//
// white goes after h8 rook and eventually gets a terrible position (more difficult positional eval), stockfish likes f3 with ~ even score
// does not see black knight to h6 at all
// r3k1nr/1p2pN1p/2p2np1/p5b1/2B3b1/2N5/PPP2PPP/R2R2K1 w kq - 2 15 
//
// do not play b8a8 here as white, stockfish likes 00, followed by bf4 ..
// 1R3b1r/p2kpp1p/N1prb1pn/q2p2N1/3P4/2P2P2/1P2Q1PP/2B1K2R w K - 2 20
// 
// don't stalemate!
// 2k5/2P5/N6p/3K3p/1P5P/6P1/8/8 w - - 1 51
//
// engine outputs garbage .. and struggles with Bf4 for 7 seconds, before realizing it drops piece
// r1b1kb1r/1p2q1pp/p2pp2n/1Np1n1N1/4P3/8/PPP1BPPP/R1BQK2R w KQkq - 0 10
//
// should find qxd3 for black easily .. does not
// position fen r2r2k1/p1p2pp1/1pn2n1p/3qp3/2P5/3P1N2/PP3PPP/R1BQR1K1 b - - 0 18
//
// should find rd5 as black, rh5 threat
// 3rr1k1/ppp1bppp/2p5/2P5/3P1Q2/4B1Pq/PP3P1P/2R2RK1 b - - 2 19
// 
// find nd5 (qa4) here as white, followed by strong k attack
// 1r2bbnr/3kpppp/3p1q2/8/2Q5/P1N1B3/1PP2PPP/R4K1R w - - 0 18
//
// find qa4+ here as white
// 1r2bb1r/3kpppp/3p1q1n/8/2Q5/P1N1B3/1PP2PPP/R5KR w - - 2 19
//
// find nd5 here (winning) stockfish eval ~4.0
// 1r2bb1r/3kpppp/3p3q/8/2Q5/P1N5/1PP2PPP/R5KR w - - 0 20
//
// find better than c1c3 .. white tries to save a-pawn, drops 2 pieces for rook .. fail. (e5f6, d1e1, d1d3(best))
// 3rrn2/1pk1p1b1/1pn1bp1p/1B1pP1p1/NB1P4/5N1P/PP3PP1/2RR2K1 w - - 4 18
//
// don't capture (or consider capturing) on b5 ... (g2g3 best try to find quickly..)
// r3kbbr/p3p2p/2pq1n1p/1p1pN3/3P4/P1N5/1PP1QPPP/2K1RB1R w kq - 5 14
//
// find something other than rf3 ? loses endgame from this position (b5b6 best?)
// r3qbk1/1pp2rpp/4p2B/1P2P3/6Q1/pP5R/P4PPP/3R2K1 w - - 1 28
//
// find qd1 here? qf3 loses pretty quickly (qg5 followed by attack at c2)
// r1bqkb1r/ppp2p1p/2n3p1/3np1NQ/2B5/8/PPPP1PPP/RNB1K2R w KQkq - 0 7
// 
// find c6 here..stop trading queens in attacking positions..
// r2k1b1r/ppp2Q1p/6p1/3Bp1q1/3n4/5PP1/PPbP3P/RNB1K2R b - - 1 13
namespace
{
  Clock timer;

  enum PositionType
    {
      POSITION_OPEN,
      POSITION_CLOSED,
      SAME_SIDE_CASTLE,
      OPPOSITE_CASTLE
    };

  struct Scores
  {
    Scores() {};
    int material_sc;
    int pawn_sc;
    int knight_sc[2];
    int bishop_sc[2];
    int rook_sc[2];
    int queen_sc[2];
    int king_sc[2];
    int threat_sc[2];
    int safety_sc[2];
    int mobility_sc[2];
    int space_sc[2];
    int center_sc[2];
    int tempo_sc[2];
    int squares_sc[2];
    int time;
  };

  struct EvalInfo
  {
    EvalInfo() {};

    int stm;
    GamePhase phase;
    U64 white_pieces;
    U64 black_pieces;
    U64 all_pieces;
    U64 empty;
    U64 white_pawns;
    U64 black_pawns;
    U64 w_pinned;
    U64 b_pinned;
    int white_ks;
    int black_ks;
    int  tempoBonus;
    MaterialEntry * me;
    PawnEntry * pe;
    PositionType position_type;
    int do_trace;
    Scores s;
  };

  // losing control of the center is roughly 1 pawn worth in material ..
  int center_weights[2][5] =
    {
      // pawn, knight, bishop, rook, queen
      { 5,4,3,2,1 },
      { 5,4,3,2,1 }
    };


  /*int threats_to_king_weights[2][5] =
    {
      // pawn, knight, bishop, rook, queen
      { 2, 1, 1, 1, 1 }, // middle-game
      { 3, 1, 1, 1, 1 }  // end-game
      };*/

  int attack_weights[2][5][6] =
    {
      {
	// pawn, knight, bishop, rook, queen, king
	{ 2, 6, 7, 9, 11, 13 },  // pawn attcks mg
	{ 1, 2, 3, 7, 9, 11 },  // knight attcks mg
	{ 1, 1, 2, 6, 9, 11 },   // bishop attcks mg
	{ 1, 1, 1, 2, 4, 11 },    // rook attcks mg
	{ 1, 1, 1, 2, 3, 11 },    // queen attcks mg
      },
      {
	// pawn, knight, bishop, rook, queen, king
	{ 2, 4, 5, 7, 9, 11 },  // pawn attcks eg
	{ 1, 2, 3, 7, 9, 11 },  // knight attcks eg
	{ 1, 1, 2, 6, 9, 11 },   // bishop attcks eg
	{ 1, 1, 1, 2, 4, 11 },    // rook attcks eg
	{ 1, 1, 1, 2, 3, 11 },    // queen attcks eg
      }
    };
  /*
  int mobility_weights[2][6] =
    {
      // pawn, knight, bishop, rook, queen, king
      { 0, 0, 0, 0, 0, 2 },
      { 2, 2, 1, 1, 1, 4 }
    };
  */

  /*
  int influence_weights[2][6] =
    {
      // pawn, knight, bishop, rook, queen, king
      { 4, 8, 8, 8, 8, 1 },
      { 4, 10, 10, 10, 10, 10 }
    };
  
  int connected_weights[2][6] =
    {
      // pawn, knight, bishop, rook, queen, king
      { 1,4,4,8,10 },
      { 1,1,1,6,6 }
    };

  int open_position_weights[2][6] =
    {
      // pawn, knight, bishop, rook, queen, king
      { 0, 1, 12, 15, 20, 0 }, // mg
      { 0, 5, 15, 25, 30, 4 } // eg
    };
  */
  int king_exposure[2] = { 5, 2 };
  int castle_weights[2] = { 2, 1 };//{ 4, 1 };
  int space_weights[2] = { 2, 1 };

  // rank or column connected score
  // rook specific	 
  //int doubled_rook_bonus[2] = { 10,16 };
  //int open_file_bonus[2] = { 3, 6 };// { 10, 16 };
  //int rank7_bonus[2] = { 4, 8 };


  int eval(Board& b);
  void traceout(EvalInfo& e);

  template<Color c>
  int eval_squares(Board& b, EvalInfo& ei);

  template<Color c>
  int eval_knights(Board& b, EvalInfo& ei);

  template<Color c>
  int eval_bishops(Board& b, EvalInfo& ei);

  template<Color c>
  int eval_rooks(Board& b, EvalInfo& ei);

  template<Color c>
  int eval_queens(Board& b, EvalInfo& ei);

  template<Color c>
  int eval_kings(Board& b, EvalInfo& ei);

  //// non-material evaluations
  //template<Color c>
  //int eval_weak_pawns(Board& b, EvalInfo& ei);

  template<Color c>
  int eval_space(Board& b, EvalInfo& ei);

  template<Color c>
  int eval_center(Board& b, EvalInfo& ei);

  template<Color c>
  int eval_threats(Board& b, EvalInfo& ei);

  // mostly dbg of material hashtable
  //int eval_material(Board& b, EvalInfo& ei);

  //template<Color c>
  //int eval_development(Board& b, EvalInfo& ei);

  //template<Color c>
  //int eval_mobility(Board& b, EvalInfo& ei);
};

///////////////////////////////////////////////////////////////////////
// external wrapper to the evaluate routine
namespace Eval
{
  int evaluate(Board& b) { return eval(b); }
};

namespace
{
  int eval(Board &b)
  {
    // fyi this is a really slow start routine...and it is called a LOT
    EvalInfo ei;

    // material entry for this position
    ei.tempoBonus = 16; // 1/3 pawn value is quite large for now.
    ei.stm = b.whos_move();
    ei.me = material.get(b);
    ei.phase = ei.me->game_phase;
    ei.white_pieces = b.colored_pieces(WHITE);
    ei.black_pieces = b.colored_pieces(BLACK);
    ei.empty = ~b.all_pieces();
    ei.all_pieces = b.all_pieces();
    ei.white_ks = b.king_square(WHITE);
    ei.black_ks = b.king_square(BLACK);
    ei.white_pawns = b.get_pieces(WHITE, PAWN);
    ei.black_pawns = b.get_pieces(BLACK, PAWN);
    ei.pe = pawnTable.get(b, ei.phase);
    ei.do_trace = false;// opts["TraceEval"];
    ei.w_pinned = b.pinned(WHITE);
    ei.b_pinned = b.pinned(BLACK);


    if (ei.do_trace)
      {
	pawnTable.debug(*ei.pe);
	ei.s.tempo_sc[WHITE] = (ei.stm == WHITE ? ei.tempoBonus : 0);
	ei.s.tempo_sc[BLACK] = (ei.stm == BLACK ? ei.tempoBonus : 0);
	ei.s.material_sc = ei.me->value;
	ei.s.pawn_sc = ei.pe->value;
	timer.start();
      }

    int score = 0;
    score += (ei.stm == WHITE ? ei.tempoBonus : -ei.tempoBonus);

    // base score 
    score += ei.me->value;

    // pawn-structure score
    score += ei.pe->value;

    // evaluate squares
    score += (eval_squares<WHITE>(b, ei) - eval_squares<BLACK>(b, ei));

    // evaluate knights
    score += (eval_knights<WHITE>(b, ei) - eval_knights<BLACK>(b, ei));

    // evaluate bishops
    score += (eval_bishops<WHITE>(b, ei) - eval_bishops<BLACK>(b, ei));

    // evaluate rook
    score += (eval_rooks<WHITE>(b, ei) - eval_rooks<BLACK>(b, ei));

    // evaluate queens
    score += (eval_queens<WHITE>(b, ei) - eval_queens<BLACK>(b, ei));

    // evaluate kings
    score += (eval_kings<WHITE>(b, ei) - eval_kings<BLACK>(b, ei));

    //-------------------------------------------------------------
    // positional considerations
    // 1. space
    // 2. weak pawns -- backward, isolated, doubled, base of pawn chain etc.
    // 3. passed pawns -- stoppable, unstoppable
    // 4. central control 
    // 5. development -- connected rooks, knights/bishops off back rank

    // evaluate space
    //score += (eval_space<WHITE>(b, ei) - eval_space<BLACK>(b, ei));

    // evaluate center control
    score += (eval_center<WHITE>(b, ei) - eval_center<BLACK>(b, ei));

    // evaluate threats -- tried to use this as is, encourages sacrificial moves
    // for no gain .. e.g. move knight in front of pawn attack to attack king etc.
    score += (eval_threats<WHITE>(b, ei) - eval_threats<BLACK>(b, ei));

    if (ei.do_trace)
      {
	timer.stop();
	ei.s.time = timer.ms();
	traceout(ei);
      }

    return b.whos_move() == WHITE ? score : -score;
  }

  // piece square tables range from (-10,10) for each piece, if each piece
  // is at its "best" square we could have an adjustment ~160 ...  best to trace this score
  // and confirm.
  template<Color c>
  int eval_squares(Board& b, EvalInfo& e)
  {
    int score = 0;
    // pawn sqs
    int * sqs = b.sq_of<PAWN>(c);
    for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
      score += square_score<c, PAWN>(e.phase, from);

    // knight sqs
    sqs = b.sq_of<KNIGHT>(c);
    for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
      score += square_score<c, KNIGHT>(e.phase, from);

    // bishop sqs
    sqs = b.sq_of<BISHOP>(c);
    for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
      score += square_score<c, BISHOP>(e.phase, from);

    // rook sqs
    sqs = b.sq_of<ROOK>(c);
    for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
      score += square_score<c, ROOK>(e.phase, from);

    // queen sqs
    sqs = b.sq_of<QUEEN>(c);
    for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
      score += square_score<c, QUEEN>(e.phase, from);

    // king sq
    sqs = b.sq_of<KING>(c);
    for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
      score += square_score<c, KING>(e.phase, from);

    if (e.do_trace)
      {
	(c == WHITE ? e.s.squares_sc[WHITE] = score : e.s.squares_sc[BLACK] = score);
      }

    // adjustments and re-weighting due to imbalances?
    return score;
  }

  //------------------------------------------------------
  // 1. right now, two knights vs. 1 knight doesn't even matter, e.g. in even-ish
  //    position, side getting an extra knight eval is coming up worse, even though
  //    both knights are about as good as the 1 knight on the other side...indicates
  //    positional eval is doing a lot of work, without adding any insight...positional eval
  //    is overwhelmed by material eval.
  // 2. check what "max" positional eval can be..
  // 3. differences between these positions (e.g.) 
  //    -- 5kb1/8/2p2p2/1p6/4N3/5P2/6PP/3K4
  //    -- 5kb1/8/2p2p2/8/2p1N3/5P2/6PP/3K4
  //    seem too large (doubled pawn penalty in eg is huge)
  // 4. focus on threats, mobility, passed pawns, and simple adjustments to piece positions, nothing more.

  template<Color c>
  int eval_knights(Board& b, EvalInfo& ei)
  {
    int score = 0;
    int *sqs = b.sq_of<KNIGHT>(c);
    int them = (c == WHITE ? BLACK : WHITE);
    U64 pinned = (c == WHITE ? ei.w_pinned : ei.b_pinned);

    for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
      {
	// knight mobility
	// 1. how many "safe" empty squares does it see
	// 2. is it pinned?
	U64 mobility = 0ULL;

	if ((SquareBB[from] & pinned)) score -= ei.tempoBonus / 2;
	{
	  mobility = PseudoAttacksBB(KNIGHT, from) & ei.empty;

	  // remove sqs attacked by enemy pawns
	  U64 tmp = ei.pe->attacks[them];
	  if (tmp)
	    {
	      if (tmp & SquareBB[from]) score -= ei.tempoBonus / 2;
	      U64 bm = mobility & tmp;
	      mobility ^= bm;
	    }
	  score += count(mobility);
	}

	// knight attacks weighted by game phase, and piece being attacked.
	U64 attacks = PseudoAttacksBB(KNIGHT, from) & ei.pe->undefended[c == WHITE ? BLACK : WHITE];//(c == WHITE ? ei.black_pieces : ei.white_pieces);
	while (attacks)
	  {
	    int to = pop_lsb(attacks);
	    int p = b.piece_on(to);
	    score += int(attack_weights[ei.phase][KNIGHT][p]); // already weighted by game phase.
	  }

	// does the position favor a knight? since ei.position_type is not very precise
	// we do not penalize for having a knight in an open position
	//if (pawn_cnt >= 13 && ei.position_type == POSITION_CLOSED) score += Value( 50 );

	/////////////////////////////////////////////////////////////////////////////////////////
	// the positional features...
	// 1. blockade backward/isolated pawns
	// 2. attacks weak pawns (backward/isolated)
	// 3. attacks "holes" in position (>= 4th rank?)
	// 4. penalize for board edge position
	// blockade squares in front of isolated and backward pawns
	U64 blockade = (c == WHITE ? (ei.pe->backwardPawns[BLACK] >> NORTH) : (ei.pe->backwardPawns[WHITE] << NORTH));
	blockade |= (c == WHITE ? (ei.pe->isolatedPawns[BLACK] >> NORTH) : (ei.pe->isolatedPawns[WHITE] << NORTH));
	blockade &= mobility;
	if (blockade) score += count(blockade);

	// outpost bonus
	if ((c == WHITE ? ROW(from) >= ROW3 : ROW(from) <= ROW6))
	  {
	    //U64 outpost = (mobility | SquareBB[from]) & (c == WHITE ? ei.pe->attacks[WHITE] : ei.pe->attacks[BLACK]) & blockade;
	    score += 1;//count(outpost);
	  }

	// evaluate threats to king 
	U64 king_threats = PseudoAttacksBB(KNIGHT, from) & KingSafetyBB[them][(them == BLACK ? ei.black_ks : ei.white_ks)];
	if (king_threats) score += 2;//threats_to_king_weights[ei.phase][KNIGHT] * count(king_threats);


	// evaluate the connected-ness of this piece (how many friendly pieces attack it)
	//U64 connected = b.attackers_of(from) & (c == WHITE ? ei.white_pieces : ei.black_pieces);
	//score += count(connected); //connected_weights[ei.phase][KNIGHT]
      }

    if (ei.do_trace)
      {
	(c == WHITE ? ei.s.knight_sc[WHITE] = score : ei.s.knight_sc[BLACK] = score);
      }
    return score;
  }

  //------------------------------------------------------
  // evaluate the bishops
  template<Color c>
  int eval_bishops(Board& b, EvalInfo& ei)
  {
    int score = 0;
    int them = (c == WHITE ? BLACK : WHITE);
    int *sqs = b.sq_of<BISHOP>(c);
    U64 pinned = (c == WHITE ? ei.w_pinned : ei.b_pinned);
    U64 mask = ei.all_pieces;

    // bishop info
    bool light_bishop = false;
    bool dark_bishop = false;

    // pawn info 
    U64 enemy_wsq_pawns = (them == WHITE ? ei.white_pawns & ColoredSquaresBB[WHITE] : ei.black_pawns & ColoredSquaresBB[WHITE]);
    U64 enemy_bsq_pawns = (them == WHITE ? ei.white_pawns & ColoredSquaresBB[BLACK] : ei.black_pawns & ColoredSquaresBB[BLACK]);
    U64 our_wsq_pawns = (them == WHITE ? ei.black_pawns & ColoredSquaresBB[WHITE] : ei.white_pawns & ColoredSquaresBB[WHITE]);
    U64 our_bsq_pawns = (them == WHITE ? ei.black_pawns & ColoredSquaresBB[BLACK] : ei.white_pawns & ColoredSquaresBB[BLACK]);

    U64 center_pawns = (ei.white_pawns | ei.black_pawns) & CenterMaskBB;
    int center_nb = count(center_pawns);

    // loop over each bishop
    for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
      {
	// what kind of bishop do we have?
	if (SquareBB[from] & ColoredSquaresBB[WHITE]) light_bishop = true;
	else dark_bishop = true;


	// mobility score
	U64 mvs = attacks<BISHOP>(mask, from);
	{
	  if ((SquareBB[from] & pinned)) score -= ei.tempoBonus / 2;

	  // collect the legal moves
	  U64 mobility = mvs & ei.empty;

	  // remove sqs attacked by enemy pawns
	  
	  U64 tmp = ei.pe->attacks[them];
	  if (tmp)
	    {
	      if (tmp & SquareBB[from]) score -= ei.tempoBonus / 2;
	      U64 bm = mobility & tmp;
	      mobility ^= bm;
	    }
	  score += count(mobility) / 4;
	}

	////////////////////////////////////////////////////////////////////////////////////
	// basic bishop eval
	// 1. base eval given by piece sq. score
	// 2. eval mobility -- remove sqs. attacked by pawns.
	// 3. sum attack bonus (all pieces (excluding pawns) the bishop is attacking).		

	// bishop attacks weighted by game phase, and piece being attacked (pawns removed (?))
	U64 attacks = mvs & ei.pe->undefended[c == WHITE ? BLACK : WHITE];//(c == WHITE ? ei.black_pieces : ei.white_pieces);
	while (attacks)
	  {
	    int to = pop_lsb(attacks);
	    int p = b.piece_on(to);
	    score += int(attack_weights[ei.phase][BISHOP][p]);
	  }

	////////////////////////////////////////////////////////////////////////
	// positional bonuses for this pieces
	// 1. open center, with not many pawns in the position
	// 2. color weakness bonus -- too few/many pawns of specific color.
	// 3. blockade/attack/protect weak squares.
	// 3. bishop pair bonus -- added at the end

	// does the position favor a bishop? -- does allow a "bad" bishop to get a bonus somtimes, look
	// at white bishop here (8/8/1pp4b/8/3Pk3/4P3/1B1K4/8), it gets the bonus below, and shouldn't.
	if (center_nb <= 2 ) score += (ei.phase == MIDDLE_GAME ? 4 : 6);

	// color penalties -- too few targets -- in endgame, should not penalize for attacking any nb of pawns!
	if (light_bishop && (enemy_wsq_pawns <= 2))  score -= (ei.phase == MIDDLE_GAME ? 2 : 4);
	if (dark_bishop && (enemy_bsq_pawns <= 2))  score -= (ei.phase == MIDDLE_GAME ? 2 : 4);

	// color penalties -- too many pawns on same color -- in theory this is not so bad in endgames
	if (light_bishop && (our_wsq_pawns >= 3)) score -= (ei.phase == MIDDLE_GAME ? 6 : 2);
	if (dark_bishop && (our_bsq_pawns >= 3))  score -= (ei.phase == MIDDLE_GAME ? 6 : 2);

	// does the bishop block an enemy passed pawn (?)
	//U64 blockade = (c == WHITE ? (ei.pe->backwardPawns[BLACK] >> NORTH) : (ei.pe->backwardPawns[WHITE] << NORTH));
	//blockade |= (c == WHITE ? (ei.pe->isolatedPawns[BLACK] >> NORTH) : (ei.pe->isolatedPawns[WHITE] << NORTH));
	////blockade &= mobility;
	//if (blockade) score += count(blockade);

	//// threats to king 
	U64 king_threats = mvs & PseudoAttacksBB(KING, (them == BLACK ? ei.black_ks : ei.white_ks));
	if (king_threats) score += 1;//threats_to_king_weights[ei.phase][BISHOP] *count(king_threats);

	// connected-ness of this piece (how many friendly pieces attack it)
	// needs to be weighted by game phase (attacking pawns in endgame is good!)
	//U64 connected = b.attackers_of(from) & (c == WHITE ? ei.white_pieces : ei.black_pieces);
	//score += count(connected);// connected_weights[ei.phase][BISHOP]
      }

    // light + dark square bishop bonus
    if (light_bishop && dark_bishop) score += int((ei.phase == MIDDLE_GAME ? 8 : 16));

    if (ei.do_trace)
      {
	(c == WHITE ? ei.s.bishop_sc[WHITE] = score : ei.s.bishop_sc[BLACK] = score);
      }
    return int(score);
  }


  //------------------------------------------------------
  // evaluate the rooks
  template<Color c>
  int eval_rooks(Board& b, EvalInfo& ei)
  {
    int score = 0;

    int them = (c == WHITE ? BLACK : WHITE);
    U64 pinned = (c == WHITE ? ei.w_pinned : ei.b_pinned);
    U64 mask = ei.all_pieces;
    int *sqs = b.sq_of<ROOK>(c);

    // pinned info/ attacker info
    U64 enemy_pawns = (c == WHITE ? ei.black_pawns : ei.white_pawns);
    U64 our_pawns = (c == WHITE ? ei.white_pawns : ei.black_pawns);
    U64 enemy_knights = (c == WHITE ? b.get_pieces(BLACK, KNIGHT) : b.get_pieces(WHITE, KNIGHT));
    U64 enemy_bishops = (c == WHITE ? b.get_pieces(BLACK, BISHOP) : b.get_pieces(WHITE, BISHOP));
    U64 pawns = (ei.white_pawns | ei.black_pawns);
    U64 rank7 = (c == WHITE ? RowBB[ROW7] : RowBB[ROW2]);

    // loop over each rook
    for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
      {

	U64 mvs = attacks<ROOK>(mask, from);

	// mobility score
	{
	  if ((SquareBB[from] & pinned)) score -= ei.tempoBonus / 2;
	  U64 mobility = mvs & ei.empty;

	  U64 tmp = ei.pe->attacks[them];
	  if (tmp)
	    {
	      if (tmp & SquareBB[from]) score -= ei.tempoBonus / 2;
	      U64 bm = mobility & tmp;
	      mobility ^= bm;
	    }
	  if (mobility) score += count(mobility) / 4;// *mobility_weights[ei.phase][ROOK];
	}

	// penalize the rook if attacked by a knight, bishop or pawn
	U64 attackers = b.attackers_of(from) & (enemy_pawns | enemy_knights | enemy_bishops);
	if (attackers) score -= ei.tempoBonus / 2;


	// rook attacks, weighted by game phase	
	U64 attacks = mvs & ei.pe->undefended[c == WHITE ? BLACK : WHITE];//(c == WHITE ? ei.black_pieces : ei.white_pieces);
	while (attacks)
	  {
	    int to = pop_lsb(attacks);
	    int p = b.piece_on(to);
	    score += int(attack_weights[ei.phase][ROOK][p]);
	  }

	// open file bonus for the rook
	U64 file_closed = ColBB[COL(from)] & pawns;
	if (!file_closed) score += 1; //int(open_file_bonus[ei.phase]); 

	U64 file_semi_closed =  ColBB[COL(from)] & our_pawns;
	if (!file_semi_closed) score += 2; //int(open_file_bonus[ei.phase])/2;

	if (SquareBB[from] & rank7) score += 2; //int(rank7_bonus[ei.phase]); 


	U64 king_threats = mvs & KingSafetyBB[them][(them == BLACK ? ei.black_ks : ei.white_ks)];
	if (king_threats) score += 1;//threats_to_king_weights[ei.phase][ROOK];// *count(king_threats);

	// evaluate the connected-ness of this piece (how many friendly pieces attack it)
	// needs to be weighted by game phase (attacking pawns in endgame is good!)
	//U64 connected = b.attackers_of(from) & (c == WHITE ? ei.white_pieces : ei.black_pieces);
	//score += count(connected); //connected_weights[ei.phase][ROOK]
      }
    if (ei.do_trace)
      {
	(c == WHITE ? ei.s.rook_sc[WHITE] = score : ei.s.rook_sc[BLACK] = score);
      }
    return score;
  }


  //------------------------------------------------------
  // evaluate the queens
  // note : certain tactical situations eval a bit faster by dropping the queen eval completely.
  template<Color c>
  int eval_queens(Board& b, EvalInfo& ei)
  {
    int score = 0;
    int them = (c == WHITE ? BLACK : WHITE);
    U64 mask = ei.all_pieces;
    int *sqs = b.sq_of<QUEEN>(c);

    // pinned info/ attacker info
    U64 pinned = (c == WHITE ? ei.w_pinned : ei.b_pinned);
    U64 enemy_pawns = (c == WHITE ? ei.black_pawns : ei.white_pawns);
    U64 enemy_knights = (c == WHITE ? b.get_pieces(BLACK, KNIGHT) : b.get_pieces(WHITE, KNIGHT));
    U64 enemy_bishops = (c == WHITE ? b.get_pieces(BLACK, BISHOP) : b.get_pieces(WHITE, BISHOP));
    U64 enemy_rooks = (c == WHITE ? b.get_pieces(BLACK, ROOK) : b.get_pieces(WHITE, ROOK));

    // loop over each queen
    for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
      {
	// it is a big deal if the queen is pinned to the king
	//if (SquareBB[from] & pinned) score -= ei.tempoBonus;

	U64 mvs = (attacks<BISHOP>(mask, from) | attacks<ROOK>(mask, from));

	// mobility -- note : mobility_weights for queen needs to be 
	// small else eval becomes very skewed.
	{
	  if (SquareBB[from] & pinned) score -= ei.tempoBonus;

	  U64 mobility = mvs & ei.empty;
	  U64 tmp = ei.pe->attacks[them];
	  if (tmp)
	    {
	      if (tmp & SquareBB[from]) score -= ei.tempoBonus;
	      U64 bm = mobility & tmp;
	      mobility ^= bm;
	    }

	  if (mobility) score += count(mobility) / 8; // helps to avoid queen traps
	}
	// penalize the queen if attacked by a knight, bishop or pawn
	U64 attackers = b.attackers_of(from) & (enemy_pawns | enemy_knights | enemy_bishops | enemy_rooks);
	if (attackers) score -= ei.tempoBonus / 2;


	// queen attacks, weighted by game phase
	U64 attacks = mvs & ei.pe->undefended[c == WHITE ? BLACK : WHITE];//(c == WHITE ? ei.black_pieces : ei.white_pieces);
	while (attacks)
	  {
	    int to = pop_lsb(attacks);
	    int p = b.piece_on(to);
	    score += int(attack_weights[ei.phase][QUEEN][p]);
	  }

	U64 king_threats = mvs & KingSafetyBB[them][(them == BLACK ? ei.black_ks : ei.white_ks)];
	if (king_threats) score += 1;//threats_to_king_weights[ei.phase][QUEEN];

	// evaluate the connected-ness of this piece (how many friendly pieces attack it)
	// needs to be weighted by game phase (attacking pawns in endgame is good!)
	//U64 connected = b.attackers_of(from) & (c == WHITE ? ei.white_pieces : ei.black_pieces);
	//score += count(connected); //connected_weights[ei.phase][QUEEN]
      }

    if (ei.do_trace)
      {
	(c == WHITE ? ei.s.queen_sc[WHITE] = score : ei.s.queen_sc[BLACK] = score);
      }
    return score;
  }

  //------------------------------------------------------
  // evaluate the kings
  template<Color c>
  int eval_kings(Board& b, EvalInfo& ei)
  {
    // ask if king is castled in middle game (?)
    int score = 0;
    int from = (c == WHITE ? ei.white_ks : ei.black_ks);
    //Color them = (c == WHITE ? BLACK : WHITE);
    U64 our_pawns = (c == WHITE ? ei.white_pawns : ei.black_pawns);
    U64 their_pawns = (c == WHITE ? ei.black_pawns : ei.white_pawns);

    //U64 mask = ei.all_pieces;
    bool castled = b.is_castled(c);

    U64 enemy_bishops = (c == WHITE ? b.get_pieces(BLACK, BISHOP) : b.get_pieces(WHITE, BISHOP));
    U64 enemy_rooks = (c == WHITE ? b.get_pieces(BLACK, ROOK) : b.get_pieces(WHITE, ROOK));
    U64 enemy_queens = (c == WHITE ? b.get_pieces(BLACK, QUEEN) : b.get_pieces(WHITE, QUEEN));
    U64 enemy_knights = (c == WHITE ? b.get_pieces(BLACK, KNIGHT) : b.get_pieces(WHITE, KNIGHT));


    // eval king safety
    {
      U64 mobility = PseudoAttacksBB(KING, from);// & ei.empty;
      //U64 sliders = (enemy_bishops | enemy_rooks | enemy_queens);
      while (mobility)
	{
	  int ksquare = pop_lsb(mobility);
	  U64 attackers = b.attackers_of(ksquare) & b.colored_pieces(c == WHITE ? BLACK : WHITE);
	  if (attackers)
	    {
	      // could be a mate threat .. make equal to roughly 1/2 pawn ?
	      if (more_than_one(attackers))
		{
		  score -= 50;
		  if (attackers & enemy_queens) score -= 50;
		  if (attackers & enemy_rooks) score -= 25;
		  if (attackers & enemy_knights) score -= 25;
		  if (attackers & enemy_bishops) score -= 25;
		}
	      score -= count(attackers);
	    }
	}

      // we almost never want the king in the corner during an endgame
      if ((from == A1 || from == A8 || from == H8 || from == H1) && ei.phase == END_GAME)
	{
	  score -= king_exposure[ei.phase];
	}
      //U64 tmp = ei.pe->attacks[them];
      //if (tmp)
      //{
      //	if (tmp & SquareBB[from]) score -= ei.tempoBonus;
      //	U64 bm = mobility & tmp;
      //	mobility ^= bm;
      //}
      //score += count(mobility);
    }

    // pawn cover around king -- based on game phase
    if (ei.phase == MIDDLE_GAME)
      {
	U64 pawn_cover = KingSafetyBB[c][from] & our_pawns;
	if (pawn_cover) score += count(pawn_cover);
      }

    // piece cover around king
    U64 piece_cover = KingSafetyBB[c][from] & (c == WHITE ? ei.white_pieces : ei.black_pieces);
    if (piece_cover) score += 1;//count(piece_cover);


    // check if castled (not perfect) -- favors "faster" castling not necessarily "safer" castling
    // better to give bonuses for rook-connectedness and pawn/piece cover so it discovers safe castle 
    // positions on its own .. how to implement?
    if (!castled) score -= castle_weights[ei.phase] * king_exposure[ei.phase];

    // update : penalize more if not castled and cannot castle
    //if (!castled && !b.can_castle(c == WHITE ? ALL_W : ALL_B)) score -= 4*castle_weights[ei.phase] * king_exposure[ei.phase];

    // note : speedup when these diag checks are removed for certain tactical test positions, but
    // play is weaker (places king in danger much sooner during game).
    if (enemy_bishops || enemy_queens)
      {
	U64 diags = BishopMask[from] & our_pawns & KingSafetyBB[c][from];
	if (diags) score += king_exposure[ei.phase];
      }
    if (enemy_rooks || enemy_queens)
      {
	U64 cols = RookMask[from] & ColBB[COL(from)] & our_pawns & KingSafetyBB[c][from];
	if (cols) score += king_exposure[ei.phase];

	cols = RookMask[from] & ColBB[COL(from)] & their_pawns;
	if (!cols) score -= king_exposure[ei.phase];

	U64 colsRight = COL(from + 1) <= COL8 ? (RookMask[from + 1] & ColBB[COL(from + 1)] & their_pawns) : 1ULL;
	if (!colsRight) score -= king_exposure[ei.phase];

	U64 colsRightRight = COL(from + 2) <= COL8 ? (RookMask[from + 2] & ColBB[COL(from + 2)] & their_pawns) : 1ULL;
	if (!colsRightRight) score -= king_exposure[ei.phase];

	U64 colsLeft = COL(from - 1) >= COL1 ? (RookMask[from - 1] & ColBB[COL(from - 1)] & their_pawns) : 1ULL;
	if (!colsLeft) score -= king_exposure[ei.phase];

	U64 colsLeftLeft = COL(from - 2) >= COL1 ? (RookMask[from - 2] & ColBB[COL(from - 2)] & their_pawns) : 1ULL;
	if (!colsLeftLeft) score -= king_exposure[ei.phase];
      }


    // idea : evaluate development and treat our "less active" pieces as dangerous to our king
    U64 back_rank = (c == WHITE ? RowBB[ROW1] : RowBB[ROW8]);
    U64 undev_pieces = back_rank & (b.get_pieces(c, KNIGHT) | b.get_pieces(c, BISHOP) | b.get_pieces(c, ROOK) | b.get_pieces(c, QUEEN));
    if (undev_pieces)
      {
	int nb_undev_pieces = count(undev_pieces);
	if (nb_undev_pieces > 2) score -= 2;// *nb_undev_pieces;
      }


    if (ei.do_trace)
      {
	(c == WHITE ? ei.s.king_sc[WHITE] = score : ei.s.king_sc[BLACK] = score);
      };
    return score;
  }


  //------------------------------------------------------
  // evaluate the space -- too simple?
  template<Color c>
  int eval_space(Board& b, EvalInfo& ei)
  {
    int score = 0;

    // compute the number of squares behind pawns
    int them = (c == WHITE ? BLACK : WHITE);
    U64 pawn_bm = (c == WHITE ? ei.white_pawns : ei.black_pawns);

    // only connected pawns.
    pawn_bm ^= ei.pe->isolatedPawns[c];

    if (!pawn_bm) return 0;


    // loop over pawns and count the squares behind them
    U64 space = 0ULL;

    while (pawn_bm)
      {
	int s = pop_lsb(pawn_bm);
	space |= SpaceBehindBB[c][s];
      }

    space &= (ColBB[COL2] | ColBB[COL3] | ColBB[COL4] | ColBB[COL5]);
    score += count(space) *space_weights[ei.phase];

    if (ei.do_trace)
      {
	(c == WHITE ? ei.s.space_sc[WHITE] = score : ei.s.space_sc[BLACK] = score);
      };
    return score;
  }

  template <Color c>
  int eval_threats(Board &b, EvalInfo& ei)
  {
    int score = 0;
    // collect all attackers
    U64 pawns = (c == WHITE ? ei.white_pawns : ei.black_pawns);
    U64 knights = (c == WHITE ? b.get_pieces(WHITE, KNIGHT) : b.get_pieces(BLACK, KNIGHT));
    U64 bishops = (c == WHITE ? b.get_pieces(WHITE, BISHOP) : b.get_pieces(BLACK, BISHOP));
    U64 queens = (c == WHITE ? b.get_pieces(WHITE, QUEEN) : b.get_pieces(BLACK, QUEEN));
    U64 rooks = (c == WHITE ? b.get_pieces(WHITE, ROOK) : b.get_pieces(BLACK, ROOK));
    U64 center_mask = (SquareBB[D4] | SquareBB[E4] | SquareBB[E5] | SquareBB[D5]);
    int enemy_ks = (c == WHITE ? ei.black_ks : ei.white_ks);

    // collect the pawn targets
    int them = (c == WHITE ? BLACK : WHITE);
    U64 all_pawns = (c == WHITE ? b.get_pieces(BLACK, PAWN) : b.get_pieces(WHITE, PAWN));
    U64 passed_pawns = ei.pe->passedPawns[them];
    U64 isolated_pawns = ei.pe->isolatedPawns[them];
    U64 backward_pawns = ei.pe->backwardPawns[them];
    U64 doubled_pawns = ei.pe->doubledPawns[them];
    U64 chain_bases = ei.pe->chainBase[them];
    //U64 king_pawns = ei.pe->kingPawns[them];
    //U64 chain_pawns = ei.pe->chainPawns[them];
    U64 undefended_pawns = ei.pe->undefended[them];


    // center pawns have been accounted for in eval_center
    U64 center_pawns = all_pawns & center_mask;

    // removed passed pawns, backward pawns and king pawns which are normally defended well.
    U64 pawn_targets = (undefended_pawns & doubled_pawns) |
      (undefended_pawns & backward_pawns) |
      (undefended_pawns & isolated_pawns) |
      (undefended_pawns & passed_pawns) |
      (undefended_pawns & center_pawns) | 
      (undefended_pawns & chain_bases); 

    // note: attack_weights for piece attacks pawn are all <= 4 so this adjustment
    // should be small
    if (pawn_targets)
      while (pawn_targets)
	{
	  score += 1; // inc score just for having the target

	  int sq = pop_lsb(pawn_targets);
	  U64 attackers = b.attackers_of(sq);

	  U64 tmp = (attackers & pawns);
	  if (tmp) score += count(tmp) *attack_weights[ei.phase][PAWN][PAWN];

	  tmp = (attackers & knights);
	  if (tmp) score += count(tmp) *attack_weights[ei.phase][KNIGHT][PAWN];

	  tmp = (attackers & bishops);
	  if (tmp) score += count(tmp) *attack_weights[ei.phase][BISHOP][PAWN];

	  tmp = (attackers & queens);
	  if (tmp) score += count(tmp) *attack_weights[ei.phase][QUEEN][PAWN];

	  tmp = (attackers & rooks);
	  if (tmp) score += count(tmp) *attack_weights[ei.phase][ROOK][PAWN];
	}

    // evaluate checks to enemy king (if any)
    U64 king_attackers = b.attackers_of(enemy_ks) & b.colored_pieces(c);

    // evaluate checks which skewer king and another piece, or check king and attack another piece/pawn
    if (king_attackers)
      {
	// evaluate double checks -- double checks are almost always serious threats .. we want to leave this outside the !attackers_of_from condition
	// so it sure to be scored.
	if (more_than_one(king_attackers)) score += 2;

	while (king_attackers)
	  {
	    int from = pop_lsb(king_attackers);

	    // imperfect .. (this will exclude the case of a defended contact check with king) .. I'm trying to consider only "safe" checks
	    // for now, i.e. the checking piece is not going to be captured on the next move .. so these threats are in some sense .. serious threats.
	    U64 attackers_of_from = b.attackers_of(from) & b.colored_pieces(them);

	    if (!attackers_of_from)
	      {
		int piece = b.piece_on(from);
		U64 attacked_squares = 0ULL;

		if (piece == KNIGHT)
		  {
		    attacked_squares = PseudoAttacksBB(KNIGHT, from) & b.colored_pieces(them);

		    // this is just a rough guess (we know there is more than one square attacked, but we could be attacking a defended pawn..)
		    // a faster/more accurate method needs a "undefended pawns/pieces bitboard" that is incrementally updated on each do-move..
		    if (more_than_one(attacked_squares)) score += 1;
		  }
		// evaluate skewer checks and double attacks from a check (imperfect)
		else if (piece != PAWN)
		  {
		    U64 mask = (piece == BISHOP ? BishopMask[from] : piece == ROOK ? RookMask[from] : QueenMask[from]);

		    // if we get here, piece is already checking the king .. if we find more than 1 attacked squares, we can flag
		    // this as either a skewer or a check+attack move...either way it is still a guess, and the attacked piece could still be defended
		    // so the bonus is quite small.
		    attacked_squares = mask & b.colored_pieces(them);
		    if (more_than_one(attacked_squares)) score += 1;
		  }
	      }
	  }
      }

    // are there any discovered check candidates in the position? These would be slider pieces that are pointed at the king,
    // but are blocked (only once) by their own friendly pieces .. should move this to incremental updates during do-undo-move.
    U64 sliders = (bishops | queens | rooks);
    if (sliders)
      while (sliders)
	{
	  int from = pop_lsb(sliders);
	  U64 between_bb = BetweenBB[from][enemy_ks] & b.colored_pieces(c); // this will always include the checking piece

	  // again this is not completely accurate, the "blocking bit" could be a pawn that is unable to move out of the way
	  // or similar.  In this case, the bonus is incorrect .. since this should be rare (in theory) we just keep the bonus small...
	  if (count(between_bb) == 2) score += 1;
	}


    // advanced passed pawns
    U64 our_passed_pawns = ei.pe->passedPawns[c];
    if (our_passed_pawns)
      {
	while (our_passed_pawns)
	  {
	    score += 1;
	    // pawns close to promotion are almost always a threat
	    int from = pop_lsb(our_passed_pawns);
	    U64 squares_until_promotion = SpaceInFrontBB[c][from];
	    if (count(squares_until_promotion) <= 3) score += 2;
	  }
      }
    // pawn forks ( should be grabbed from the pawn table?)..


    if (ei.do_trace)
      {
	(c == WHITE ? ei.s.threat_sc[WHITE] = score : ei.s.threat_sc[BLACK] = score);
      };

    return score;
  }
  //--------------------------------------------------------------------
  // evaluate center -- impacts opening play
  template<Color c>
  int eval_center(Board &b, EvalInfo& ei)
  {
    int score = 0;
    U64 bigCenter = CenterMaskBB;

    U64 pawns = (c == WHITE ? ei.white_pawns : ei.black_pawns);
    
    U64 knights = (c == WHITE ? b.get_pieces(WHITE, KNIGHT) : b.get_pieces(BLACK, KNIGHT));
    U64 bishops = (c == WHITE ? b.get_pieces(WHITE, BISHOP) : b.get_pieces(BLACK, BISHOP));
    U64 queens = (c == WHITE ? b.get_pieces(WHITE, QUEEN) : b.get_pieces(BLACK, QUEEN));
    U64 rooks = (c == WHITE ? b.get_pieces(WHITE, ROOK) : b.get_pieces(BLACK, ROOK));
    
    U64 attackers_of_big_center = 0ULL;
    while (bigCenter)
      {
	int s = pop_lsb(bigCenter);
	U64 tmp = b.attackers_of(s) & b.colored_pieces(c);
	if (tmp) attackers_of_big_center |= tmp;
      }

    // pawn weight -- weight pawns a little more than normal pieces
    // this is to boost opening play..hedwig doesn't really fight for central control...at all.
    if (pawns)
      {
	U64 pawn_bm = pawns & attackers_of_big_center;
	if (pawn_bm) score += count(pawn_bm) * center_weights[ei.phase][PAWN];
      }
    
    // knight weight
    if (knights)
      {
	U64 knight_bm = knights & attackers_of_big_center;
    	if (knight_bm) score += count(knight_bm);// *center_weights[ei.phase][KNIGHT];
      }
    
    //// bishop weight
    if (bishops)
      {
	U64 bish_bm = bishops & attackers_of_big_center;
    	if (bish_bm) score += count(bish_bm);// *center_weights[ei.phase][BISHOP];
      }
    
    //// rook weight
    if (rooks)
      {
	U64 rook_bm = rooks & attackers_of_big_center;
    	score += count(rook_bm);// *center_weights[ei.phase][ROOK];
      }
    
    // queen weight
    if (queens)
      {
    	U64 queen_bm = queens & attackers_of_big_center;
    	score += count(queen_bm);// * center_weights[ei.phase][QUEEN];
      }
    
    // give a small bonus for having pawns in the center (?)
    // this can help opening play avoid odd opening moves
    U64 small_center_pawns = pawns & CenterMaskBB;
    if (small_center_pawns) score += count(small_center_pawns)*center_weights[ei.phase][PAWN];

    if (ei.do_trace)
      {
	(c == WHITE ? ei.s.center_sc[WHITE] = score : ei.s.center_sc[BLACK] = score);
      };
    return score;
  }

  ////------------------------------------------------------
  //// evaluate development
  ////
  //template<Color c, bool debug>
  //Value eval_development(Board &b, EvalInfo &ei)
  //{

  //	Value score      = Value(0);
  //	BitMap knights   = (c == WHITE ? b.get_pieces(WHITE,KNIGHT) : b.get_pieces(BLACK, KNIGHT) );
  //	BitMap bishops   = (c == WHITE ? b.get_pieces(WHITE,BISHOP) : b.get_pieces(BLACK,BISHOP) ) ;   
  //	BitMap queens    = (c == WHITE ? b.get_pieces(WHITE,QUEEN)  : b.get_pieces(BLACK,QUEEN));
  //	BitMap backRank  = (c == WHITE ? board_row[ROW_1]           : board_row[ROW_8]);
  //	BitMap pawns     = (c == WHITE ? ei.white_pawns : ei.black_pawns);
  //	BitMap rank2mask = (c == WHITE ? (set_bit(E2) | set_bit(D2)) : (set_bit(E7) | set_bit(D7)));
  //	//Byte   castle    = (c == WHITE ? b.get_castle_rights(WHITE) : b.get_castle_rights(BLACK) );
  //	//byte   ks_c      = (c == WHITE ? set_bit_cr(W_KS) : set_bit_cr(B_KS));
  //	//byte   qs_c      = (c == WHITE ? set_bit_cr(W_QS) : set_bit_cr(B_QS));
  //	//Square ks        = (c == WHITE ? ei.white_ks : ei.black_ks)
  //	//bool canCastle   = (castle ? true : false);


  //	// penalize undeveloped knights
  //	BitMap undeveloped_knights = backRank & knights;
  //	score -= Value( 100*pop_count64(&undeveloped_knights));

  //	// penalize undeveloped bishops
  //	BitMap undeveloped_bishops = backRank & bishops;
  //	score -= Value( 60*pop_count64(&undeveloped_bishops));

  //	// penalize undeveloped queens
  //	BitMap undeveloped_queens  = backRank & queens;
  //	score -= Value( pop_count64(&undeveloped_queens));

  //	// penalize undeveloped center pawns
  //	BitMap undeveloped_pawns = rank2mask & pawns;
  //	score -= Value(35*pop_count64(&undeveloped_pawns));

  //	// rook connectedness and castled?
  //	// 
  //	//if (canCastle) score -= Value(50);


  //	return Value(score);

  //	//std::cout << " c = " << (c == WHITE ? "white":"black") << " devel score = " << score << std::endl;
  //}

  void traceout(EvalInfo& ei)
  {
    printf("\n");
    printf("+=======================================================+\n");
    printf("| Material    |\t --- \t|\t --- \t|\t %d \t|\n", ei.me->value);
    printf("| Pawns       |\t --- \t|\t --- \t|\t %d \t|\n", ei.pe->value);
    printf("| Squares     |\t %d \t|\t %d \t|\t %d \t|\n", ei.s.squares_sc[WHITE], ei.s.squares_sc[BLACK], ei.s.squares_sc[WHITE] - ei.s.squares_sc[BLACK]);
    printf("| Tempo       |\t %d \t|\t %d \t|\t %d \t|\n", ei.s.tempo_sc[WHITE], ei.s.tempo_sc[BLACK], ei.s.tempo_sc[WHITE] - ei.s.tempo_sc[BLACK]);
    printf("| Knights     |\t %d \t|\t %d \t|\t %d \t|\n", ei.s.knight_sc[WHITE], ei.s.knight_sc[BLACK], ei.s.knight_sc[WHITE] - ei.s.knight_sc[BLACK]);
    printf("| Bishops     |\t %d \t|\t %d \t|\t %d \t|\n", ei.s.bishop_sc[WHITE], ei.s.bishop_sc[BLACK], ei.s.bishop_sc[WHITE] - ei.s.bishop_sc[BLACK]);
    printf("| Rooks       |\t %d \t|\t %d \t|\t %d \t|\n", ei.s.rook_sc[WHITE], ei.s.rook_sc[BLACK], ei.s.rook_sc[WHITE] - ei.s.rook_sc[BLACK]);
    printf("| Queens      |\t %d \t|\t %d \t|\t %d \t|\n", ei.s.queen_sc[WHITE], ei.s.queen_sc[BLACK], ei.s.queen_sc[WHITE] - ei.s.queen_sc[BLACK]);
    printf("| Kings       |\t %d \t|\t %d \t|\t %d \t|\n", ei.s.king_sc[WHITE], ei.s.king_sc[BLACK], ei.s.king_sc[WHITE] - ei.s.king_sc[BLACK]);
    //printf("| Space       |\t %d \t|\t %d \t|\t %d \t|\n", ei.s.space_sc[WHITE], ei.s.space_sc[BLACK], ei.s.space_sc[WHITE] - ei.s.space_sc[BLACK]);
    printf("| Center      |\t %d \t|\t %d \t|\t %d \t|\n", ei.s.center_sc[WHITE], ei.s.center_sc[BLACK], ei.s.center_sc[WHITE] - ei.s.center_sc[BLACK]);
    printf("| Threats     |\t %d \t|\t %d \t|\t %d \t|\n", ei.s.threat_sc[WHITE], ei.s.threat_sc[BLACK], ei.s.threat_sc[WHITE] - ei.s.threat_sc[BLACK]);
    printf("| Eval time   |\t -- \t|\t -- \t|\t %d \t|\n", ei.s.time);
  }

}; // end namespace
