#include "evaluate.h"
#include "definitions.h"
#include "squares.h"
#include "globals.h"
#include "utils.h"
#include "magic.h"
#include "material.h"
#include "pawns.h"
#include "opts.h"

using namespace Globals;
using namespace Magic;

namespace
{
  struct EvalInfo
  {
    int stm;
    GamePhase phase;
    U64 white_pieces;
    U64 black_pieces;
    U64 all_pieces;
    U64 empty;
    U64 white_pawns;
    U64 black_pawns;
    U64 w_pinned;
    U64 w_pinners;
    U64 b_pinned;
    U64 b_pinners;
    int white_ks;
    int black_ks;
    int tempoBonus;
    MaterialEntry * me;
    PawnEntry * pe;
    bool trace;
  };

  // bonuses/scores/definitions/positional weights
#define S(x,y) pick_score(x,y)
  inline int pick_score(int x, int y) { return (x + 1000 * y); }
  
  int PieceMobility[PIECES][32] =
    { {}, // pawn
      {S(-20,-30),S(-15,0),S(-10,0),S(5,0),S(10,15),S(20,30),S(40,60),S(80,100)}, // knight
      
      {S(-20,-50),S(-30,-50),S(-30,-50),S(-20,-45),S(-15,-30),S(-10,-35),S(0,-10),
       S(10,5),S(20,20),S(30,30),S(40,50),S(50,60),S(60,70),S(80,100)}, // bishop

      {S(-20,-50),S(-30,-50),S(-30,-50),S(-20,-45),S(-15,-30),S(-10,-35),S(0,-10),
       S(10,5),S(20,20),S(30,30),S(40,50),S(50,60),S(60,70),S(80,100)}, // rook

      // middle game mobility scores for queen are conservative
      {S(-5,-100),S(-5,-100),S(-5,-100),S(-5,-100),S(0,-90),S(0,-80),S(0,-70),
       S(0,-60),S(5,-50),S(5,-40),S(5,-30),S(10,-20),S(10,-10),S(10,0), 
       S(20,10),S(20,20),S(20,30),S(30,40),S(30,40),S(40,50),S(50,60),
       S(60,70),S(70,80),S(80,90),S(90,100),S(100,110),S(110,120),S(120,130)}, // queen      
      {} // king 
    };
  
  // piece attacks : [attacker][victim]
  int PieceAttacks[5][6] =
    {
      // pawn,    knight,   bishop,   rook,       queen,       king
      { S(30,35), S(40,45), S(50,55), S(60,65), S(95, 100), S(100, 110) }, //pawn
      { S(10,22), S(35,42), S(42,44), S(65,65), S(95, 100), S(100, 100) }, //knight
      { S(10,22), S(35,42), S(42,44), S(65,65), S(95, 100), S(100, 100) }, //bishop
      { S(10,22), S(35,42), S(32,34), S(45,50), S(90, 100), S(95, 100) }, //rook
      { S(10,10), S(30,33), S(30,33), S(45,45), S(50, 55), S(95, 100) }, //queen
    };
  
  // [pinned piece][piece pinned to]
  int PinPenalty[PIECES-1][PIECES] =
    {
      { S(0,0), S(-2,-3), S(-3,-4), S(-6,-10), S(-15,-20), S(-18,-25) }, // pawn
      { S(0,0), S(-1,-2) , S(-2,-3), S(-10,-12), S(-20,-30), S(-30,-40) }, // knight
      { S(0,0), S(-1,-2) , S(-1,-2), S(-10,-12), S(-20,-30), S(-30,-40) }, // bishop
      { S(0,0), S(-1,-2) , S(-1,-2), S(-1,-2), S(-20,-30), S(-40,-50) }, // rook
      { S(0,0), S(-1,-2) , S(-1,-2), S(-1,-2), S(-1,-2), S(-50,-60) } // queen
    };

  // king pressure pawn, knight, bishop, rook, queen
  int KingPressure[PIECES-1] = { S(20, 20), S(45,55), S(55,65), S(75,80), S(95,100) };

  int MaterialMG[5] = { PawnValueMG, KnightValueMG, BishopValueMG, RookValueMG, QueenValueMG };
  int MaterialEG[5] = { PawnValueEG, KnightValueEG, BishopValueEG, RookValueEG, QueenValueEG };

  int ClosedPositionBonus[2] = {35, 45};
  int OpenPositionBonus[2] = {45, 50};
  int BishopFriendlyPawnPenalty[2] = {-20, -35};
  int BishopFriendlyPawnBonus[2] = {20, 35};
  int BishopEnemyPawnPenalty[2] = {-20, -35};
  int BishopEnemyPawnBonus[2] = {20, 35};
#undef S

  /*main evaluation methods*/
  int eval(Board& b);
  
  // track scores - order: material score, pawn score, square score, piece score,
  // ... ?
  //std::vector<int> scores[2];

  void traceout(EvalInfo& e); // debug output
  
  template<Color c> int eval_squares(Board& b, EvalInfo& ei);

  /*piece specific evaluations */
  template<Color c> int eval_pieces(Board&b, EvalInfo& ei);
  template<Color c, Piece p> int eval_piece(Board& b, EvalInfo& ei);
  template<Color c, Piece p> int eval_mobility(Board& b, EvalInfo& ei, int from, U64& quiets);
  template<Color c, Piece p> int eval_attacks(Board& b, EvalInfo& ei, int from, U64& attacks);
  template<Color c, Piece p> int eval_pins(Board& b, EvalInfo& ei, int from);
  template<Color c> int eval_trapped(Board& b, EvalInfo& ei);
  template<Color c, Piece p> int eval_piece_tempo(Board&b, EvalInfo& ei, int from);
  template<Color c> int eval_color_complexes(Board&b, EvalInfo&ei, int from, int pawnCount);
  /* positional/control */
  template<Color c> int eval_space(Board& b, EvalInfo& ei);
  template<Color c> int eval_center_pressure(Board& b, EvalInfo& ei); // pawns and minors only (?)
  template<Color c> int eval_open_files(Board& b, EvalInfo& ei);

  /*king safety specific*/
  template<Color c> int eval_king(Board& b, EvalInfo& ei);
  template<Color c, Piece p> int eval_king_pressure(Board& b, EvalInfo& ei);
  template<Color c> int eval_pawn_storms(Board& b, EvalInfo& ei);

  /*passed pawn candidates*/
  template<Color c> int eval_passers(Board& b, EvalInfo& ei);

  /*misc utility*/
  template<Color c> int eval_control(Board& b, EvalInfo& ei, int sq);
  template<Piece p> int mob_score(int phase, int nb);
  template<Piece p1, Piece p2> int pin_score(int phase);
  template<Piece p1> int attack_score(int phase, int p2);
  template<Piece p> int king_pressure_score(int phase);
};

namespace Eval
{
  int evaluate(Board& b) { return eval(b); }
};

namespace
{
  int eval(Board &b)
  {
    EvalInfo ei;
    ei.trace = options["trace eval"];

    ei.stm = b.whos_move();
    ei.me = material.get(b);
    ei.phase = ei.me->game_phase;
    ei.tempoBonus = (ei.phase == MIDDLE_GAME ? 16 : 22);
    ei.white_pieces = b.colored_pieces(WHITE);
    ei.black_pieces = b.colored_pieces(BLACK);
    ei.empty = ~b.all_pieces();
    ei.all_pieces = b.all_pieces();
    ei.white_ks = b.king_square(WHITE);
    ei.black_ks = b.king_square(BLACK);
    ei.white_pawns = b.get_pieces(WHITE, PAWN);
    ei.black_pawns = b.get_pieces(BLACK, PAWN);
    ei.pe = pawnTable.get(b, ei.phase);
    ei.w_pinned = b.pinned(WHITE);
    ei.b_pinned = b.pinned(BLACK);

    U64 pieceBB = b.whos_move() == WHITE ? ei.white_pieces ^ ei.white_pawns : ei.black_pieces ^ ei.black_pawns;
    int nb_pieces = b.whos_move() == WHITE ? count(pieceBB) : count(pieceBB);

    int score = 0;    
    score += (ei.stm == WHITE ? ei.tempoBonus : -ei.tempoBonus);

    score += (eval_squares<WHITE>(b, ei) - eval_squares<BLACK>(b, ei));
    
    //int s1 = (eval_squares<WHITE>(b, ei) - eval_squares<BLACK>(b, ei))/4;

    score += (eval_pieces<WHITE>(b,ei) - eval_pieces<BLACK>(b, ei));   

    //int s2 = (eval_pieces<WHITE>(b,ei) - eval_pieces<BLACK>(b, ei));   

    score += ei.pe->value; //scores[0].push_back(ei.pe->value); scores[1].push_back(ei.pe->value);
    
    //int s3 = ei.pe->value;

    // max_score ~ 100 for each eval component
    // components - (attacks, square score, mobility) ~ expect 10%, 30%, 20% of pieces to achieve a score ~100
    // max allowed positional eval ~ +/-370, rescale score by 370 / max_score
    // this excludes passed pawns, and material, and king safety evals
    // *is* possible to have scaling > 1 and amplify positional eval .. although this does not seem to hurt things.

    float f = 100.0 * nb_pieces * ( 0.1  + 0.3 + 0.2);
    float scaling = f == 0 ? 1 : (300.0 / f);

    //if (score > 170) score = 170;
    //if (score < -170) score = -170;

    score *= scaling;

    score += ei.me->value; //scores[0].push_back(ei.me->value); scores[1].push_back(ei.me->value); // throws std::bad_alloc
    //int s4 = ei.me->value;
    //b.print();
    //printf("move=%s, nb pieces= %d, scores = (%d, %d, %d, %d), final = %d\n", b.whos_move() == WHITE ? "white" : "black", nb_pieces, s1, s2, s3, s4, score);
    
    return (b.whos_move() == WHITE ? score : -score);
  }

  template<Color c> 
  int eval_pieces(Board&b, EvalInfo& ei)
  {
    int score = 0;
    score += eval_piece<c,KNIGHT>(b, ei);
    score += eval_piece<c,BISHOP>(b, ei);
    score += eval_piece<c,ROOK>(b, ei);
    score += eval_piece<c,QUEEN>(b, ei);
    return score;
  }

  template<Color c, Piece p>
  int eval_piece(Board& b, EvalInfo& ei)
  {
    int score = 0;
    int *sqs = 0; sqs = b.sq_of<p>(c);
    if (!sqs) return score;
    int pawnCount = 0;
    U64 pawns = (ei.white_pawns | ei.black_pawns);
    if (pawns) pawnCount = count(pawns);
    int bishopCount = 0;
    int eks = (c == WHITE ? ei.black_ks : ei.white_ks);
    int them = (c == WHITE ? BLACK : WHITE);
    U64 kingRegion = KingSafetyBB[them][eks];

    for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs)
      {
	// collect moves
	U64 mvs = 0ULL; U64 quiet_mvs = 0ULL; U64 captr_mvs = 0ULL;
	if (p == KNIGHT) mvs = PseudoAttacksBB(KNIGHT, from);
	else if (p == BISHOP || p == ROOK) mvs = attacks<p>(ei.all_pieces, from);
	else mvs = (attacks<ROOK>(ei.all_pieces, from) | attacks<BISHOP>(ei.all_pieces, from)); // queen
	
	quiet_mvs = mvs & ei.empty;
	captr_mvs = mvs & (c == WHITE ? ei.black_pieces : ei.white_pieces);

	// 1. attacker score
	score += eval_attacks<c,p>(b, ei, from, captr_mvs);	
	
	// 2. mobility (using built pin bitboards)
	score += eval_mobility<c,p>(b, ei, from, quiet_mvs);	

	// 3. pins (part of mobility)
	if (c == b.whos_move()) score += eval_pins<c,p>(b, ei, from); // note: these are *negative* scores

	// 4. tempo (attacked by lesser valued piece)
	score += eval_piece_tempo<c,p>(b, ei, from);

	// 5. positional (game phase)	
	
	if (ei.pe->blockedCenter && 
	    pawnCount >= 12 && 
	    p == KNIGHT) 
	  score += ClosedPositionBonus[ei.phase];
	
	if (!ei.pe->blockedCenter && 
	    pawnCount < 11 && 
	    p == BISHOP)
	  score += OpenPositionBonus[ei.phase];

	if (p == BISHOP) 
	  {
	    score += eval_color_complexes<c>(b, ei, from, pawnCount);  // apply penalties
	    ++bishopCount;
	  }
	
	// 6. king pressure
	U64 kingAttacks = (quiet_mvs | captr_mvs) & kingRegion;
	if (kingAttacks) score += king_pressure_score<p>(ei.phase);// count(kingAttacks);
	
	// 7. connectedness
      }
    if (bishopCount >= 2) score += (ei.phase == MIDDLE_GAME ? 30 : 50);
    //scores[c].push_back(score);
    return score;
  }

  // todo fixme : this corrupts the eval with skewed mobility scores (bonus/penalties are very large)
  // need to cut/prevent overflows and/or rescale
  // todo fixme : pass in the quiet mvs array 
  template<Color c, Piece p>
  int eval_mobility(Board& b, EvalInfo& ei, int from, U64& mvs)
  {
    int score = 0;
   
    // remove sqs attacked by pawns
    U64 bb = mvs & ei.pe->attacks[c==WHITE?BLACK:WHITE];
    mvs ^= bb;

    // restrict in case of pins

    score += mob_score<p>(ei.phase, count(mvs));    
    return score;
  }

  template<Color c, Piece p> 
  int eval_piece_tempo(Board&b, EvalInfo& ei, int from)
  {
    int score = 0;
    int them = c == WHITE ? BLACK : WHITE;
    U64 pattacks = ei.pe->attacks[them];
    U64 minors = b.get_pieces(them, BISHOP) | b.get_pieces(them, KNIGHT);
    U64 rooks = b.get_pieces(them, ROOK);
    U64 queens = b.get_pieces(them, QUEEN);
    if (SquareBB[from] & pattacks) score -= ei.tempoBonus;
    if (p >= ROOK)
      {
	U64 attackers = b.attackers_of(from);
	if (p == ROOK && (attackers & minors)) score -= ei.tempoBonus;
	if (p == QUEEN && ((attackers & minors) || (attackers & rooks))) score -= ei.tempoBonus;
      }
    return score;
  }

  template<Color c> 
  int eval_color_complexes(Board&b, EvalInfo&ei, int from, int pawnCount)
  {
    int score = 0;
    bool light_bishop = false;
    bool dark_bishop = false;
    int them = c == WHITE ? BLACK : WHITE;
    // pawn info 
    U64 enemy_wsq_pawns = (them == WHITE ? ei.white_pawns & ColoredSquaresBB[WHITE] : ei.black_pawns & ColoredSquaresBB[WHITE]);
    U64 enemy_bsq_pawns = (them == WHITE ? ei.white_pawns & ColoredSquaresBB[BLACK] : ei.black_pawns & ColoredSquaresBB[BLACK]);
    U64 our_wsq_pawns = (them == WHITE ? ei.black_pawns & ColoredSquaresBB[WHITE] : ei.white_pawns & ColoredSquaresBB[WHITE]);
    U64 our_bsq_pawns = (them == WHITE ? ei.black_pawns & ColoredSquaresBB[BLACK] : ei.white_pawns & ColoredSquaresBB[BLACK]);

    U64 center_pawns = (ei.white_pawns | ei.black_pawns) & CenterMaskBB;
    U64 pawns = (ei.white_pawns | ei.black_pawns) & CenterMaskBB;
    int center_nb = count(center_pawns);
    
    if (light_bishop && (enemy_wsq_pawns <= 2)) score += BishopEnemyPawnPenalty[ei.phase]; 
    if (dark_bishop && (enemy_bsq_pawns <= 2)) score += BishopEnemyPawnPenalty[ei.phase]; 
    
    // color penalties -- too many pawns on same color -- in theory this is not so bad in endgames
    if (light_bishop && (our_wsq_pawns >= 4)) score += BishopFriendlyPawnPenalty[ei.phase]; 
    if (dark_bishop && (our_bsq_pawns >= 4)) score += BishopFriendlyPawnPenalty[ei.phase]; 
    return score;
  }

  template<Color c, Piece p>
  int eval_attacks(Board& b, EvalInfo& ei, int from, U64& attacks)
  {
    int score = 0;
    int them = (c == WHITE ? BLACK : WHITE);
    int ks = (c == WHITE ? ei.white_ks : ei.black_ks);

    while (attacks)
      {
	int to = pop_lsb(attacks);
	score += attack_score<p>(ei.phase, b.piece_on(to));

	// pins piece to king ..
	/*
	U64 pinned_to_king = 0ULL; pinned_to_king = SquareBB[to] & (them == WHITE ? ei.w_pinned : ei.b_pinned);
	if (pinned_to_king) 
	  {
	    disp = true;
	    score -= pin_score<p, KING>(ei.phase); // pin scores are negative
	  }
	*/
	// pins piece to queen
	/*
	int * q_sqs = b.sq_of<QUEEN>(c);
	for (int f = *++q_sqs; f != SQUARE_NONE; f = *++q_sqs)
	  {
	    U64 pinned_to_queen = b.pinned_to(f);
	    if (!pinned_to_queen) continue;
	    score += pin_score<p, QUEEN>(ei.phase);
	  }
	*/
      }
    /*
    if (disp)
     { 
	printf("...c(%s), p(%d), attk-score(%d)\n",c==WHITE?"white":"black",p,score);
	printf("\n");
      }
    */
    return score;
  }

  template<Color c, Piece p>
  int eval_pins(Board& b, EvalInfo& ei, int from)
  {
    // nb: only checking pins against king and queen for now
    int score = 0;
    int them = (c == WHITE ? BLACK : WHITE);
    int ks = (c == WHITE ? ei.white_ks : ei.black_ks);
    U64 sliders = b.get_pieces(them, BISHOP) | b.get_pieces(them, ROOK) | b.get_pieces(them, QUEEN);
    U64 attackers = b.attackers_of(from) & sliders;
    //bool disp = false;
    
    // ensure value of pinning piece > pinned piece 
    // means we should rescale the penalty .. especially if we can capture the pinning piece etc.
    U64 pinned_to_king = 0ULL; pinned_to_king = SquareBB[from] & (c == WHITE ? ei.w_pinned: ei.b_pinned);
    if (pinned_to_king) 
      {
	int s = pop_lsb(attackers); // imperfect for multiple pinning pieces!
	int mdiff = MaterialMG[p] - MaterialMG[b.piece_on(s)];
	if (mdiff > 0) 
	  {
	    //printf("diff = %d\n",mdiff);
	    //disp = true;
	    score += pin_score<p, KING>(ei.phase);// - mdiff/15;
	  }
	else if (mdiff == 0) score = -20;
	else score = -10;
      }
    
    /*
    int * q_sqs = b.sq_of<QUEEN>(c);
    if (!q_sqs) return score;
    for (int f = *++q_sqs; f != SQUARE_NONE; f = *++q_sqs)
      {
	U64 pinned_to_queen = b.pinned_to(f);
	if (!pinned_to_queen) continue;
	score += pin_score<p, QUEEN>(ei.phase);
      }
    */
    /*
    if (disp)
      {
	b.print();
	printf("..pinned score (%s,%d) = %d\n",c==WHITE?"white":"black", p, score);
      }
    */
    //if (score != 0) printf("score=%d\n",score);
    return score;
  }

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
    score += square_score<c, KING>(e.phase, (c == WHITE ? e.white_ks : e.black_ks));

    //scores[c].push_back(score);
    return score;
  }

  // misc
  template<Piece p> int mob_score(int phase, int nb)
  {
    int ss = PieceMobility[p][nb];
    int eg = ss / 1000;    
    int mg = ss - 1000 * round( (float) ss / (float) 1000);
    return (phase == MIDDLE_GAME ? mg : eg); 
  }
  // piece p1 is pinned to p2
  template<Piece p1, Piece p2> int pin_score(int phase)
  {
    int ss = PinPenalty[p1][p2];
    int eg = ss / 1000;    
    int mg = ss - 1000 * round( (float) ss / (float) 1000);
    return (phase == MIDDLE_GAME ? mg : eg); 
  }
  template<Piece p1> int attack_score(int phase, int p2)
  {
    int ss = PieceAttacks[p1][p2];
    int eg = ss / 1000;    
    int mg = ss - 1000 * round( (float) ss / (float) 1000);
    return (phase == MIDDLE_GAME ? mg : eg); 
  }
  template<Piece p> int king_pressure_score(int phase)
  {
    int ss = KingPressure[p];
    int eg = ss / 1000;    
    int mg = ss - 1000 * round( (float) ss / (float) 1000);
    return (phase == MIDDLE_GAME ? mg : eg); 
  }
};
