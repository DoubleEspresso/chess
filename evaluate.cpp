#ifdef __linux
#include <cstring>
#endif
#include "evaluate.h"
#include "endgame.h"
#include "squares.h"
#include "globals.h"
#include "utils.h"
#include "magic.h"
#include "opts.h"
#include "evaltables.h"

using namespace Globals;
using namespace Magic;

namespace {
  Clock timer;
  int eval(Board& b);
  void traceout(EvalInfo& e);
  template<Color c> int eval_squares(Board& b, EvalInfo& ei);
  template<Color c> int eval_knights(Board& b, EvalInfo& ei);
  template<Color c> int eval_bishops(Board& b, EvalInfo& ei);
  template<Color c> int eval_rooks(Board& b, EvalInfo& ei);
  template<Color c> int eval_queens(Board& b, EvalInfo& ei);
  template<Color c> int eval_kings(Board& b, EvalInfo& ei);
  template<Color c> int eval_space(Board& b, EvalInfo& ei);
  template<Color c> int eval_center(Board& b, EvalInfo& ei);
  template<Color c> int eval_threats(Board& b, EvalInfo& ei);
  template<Color c> int eval_pawn_levers(Board& b, EvalInfo& ei);

  /* material scaling */
  template<Color c> int eval_imbalance(Board& b, EvalInfo& ei);

  /* todo */
  template<Color c> int eval_xray_bishop(Board& b, EvalInfo& ei);
  template<Color c> int eval_xray_rook(Board& b, EvalInfo& ei);
  template<Color c> int eval_xray_queen(Board& b, EvalInfo& ei);
  
  int eval(Board &b) {
    EvalInfo ei;
    ei.tempoBonus = 8;
    ei.stm = b.whos_move();
    ei.me = material.get(b);
    ei.phase = ei.me->game_phase;
    ei.pieces[WHITE] = b.colored_pieces(WHITE);
    ei.pieces[BLACK] = b.colored_pieces(BLACK);
    ei.empty = ~b.all_pieces();
    ei.all_pieces = b.all_pieces();
    ei.kingsq[WHITE] = b.king_square(WHITE);
    ei.kingsq[BLACK] = b.king_square(BLACK);
    ei.pawns[WHITE] = b.get_pieces(WHITE, PAWN);
    ei.pawns[BLACK] = b.get_pieces(BLACK, PAWN);
    ei.backrank[WHITE] = RowBB[ROW1];
    ei.backrank[BLACK] = RowBB[ROW8];
    ei.all_pawns = ei.pawns[WHITE] | ei.pawns[BLACK];
    ei.pe = pawnTable.get(b, ei.phase);
    ei.do_trace = 0;// options["trace eval"];
    ei.endgame_eval = 0; // debug for now
    for (int s = 0; s < 64; ++s) {
      ei.pinners[WHITE][s] = PIECE_NONE; ei.pinners[BLACK][s] = PIECE_NONE;
    }
    ei.pinned[WHITE] = b.pinned(WHITE, ei.pinners[BLACK]);
    ei.pinned[BLACK] = b.pinned(BLACK, ei.pinners[WHITE]);
    memset(ei.ks, 0, 2 * sizeof(KingSafety));
    //U64 defended = b.non_pawn_material(WHITE) & ei.pe->attacks[WHITE];
    //ei.weak_enemies[WHITE] = b.non_pawn_material(WHITE) ^ defended;
    //defended = b.non_pawn_material(BLACK) & ei.pe->attacks[BLACK];
    //ei.weak_enemies[BLACK] = b.non_pawn_material(BLACK) ^ defended;

    if (ei.do_trace) {
      ei.s.tempo_sc[WHITE] = (ei.stm == WHITE ? ei.tempoBonus : 0);
      ei.s.tempo_sc[BLACK] = (ei.stm == BLACK ? ei.tempoBonus : 0);
      ei.s.material_sc = ei.me->value;
      ei.s.pawn_sc = ei.pe->value;
      timer.start();
    }
    int score = 0;
    score += (ei.stm == WHITE ? ei.tempoBonus : -ei.tempoBonus);
    score += ei.me->value;
    score += ei.pe->value;
    score += (eval_squares<WHITE>(b, ei) - eval_squares<BLACK>(b, ei));
    score += (eval_knights<WHITE>(b, ei) - eval_knights<BLACK>(b, ei));
    score += (eval_bishops<WHITE>(b, ei) - eval_bishops<BLACK>(b, ei));
    score += (eval_rooks<WHITE>(b, ei) - eval_rooks<BLACK>(b, ei));
    score += (eval_queens<WHITE>(b, ei) - eval_queens<BLACK>(b, ei));
    score += (eval_kings<WHITE>(b, ei) - eval_kings<BLACK>(b, ei));
    //score += (eval_center<WHITE>(b, ei) - eval_center<BLACK>(b, ei));
    score += (eval_space<WHITE>(b, ei) - eval_space<BLACK>(b, ei));
    score += (eval_threats<WHITE>(b, ei) - eval_threats<BLACK>(b, ei));
    score += (eval_pawn_levers<WHITE>(b, ei) - eval_pawn_levers<BLACK>(b, ei));

    // debug specialized endgame eval
    if (ei.phase == END_GAME && ei.me->endgame_type != KxK) {
      //int e_score = Endgame::evaluate(b, ei);
      
      // interpolate (?) todo, if we get here, the position is definitely
      // a pure endgame position, but we still want to capture the material eval
      // egame square scores, and piece activities
      //score = score + e_score;
    }
    
    if (ei.do_trace) {
      timer.stop();
      ei.s.time = timer.ms();
      traceout(ei);
    }

    return b.whos_move() == WHITE ? score : -score;
  }

  template<Color c> int eval_squares(Board& b, EvalInfo& e) {
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
    
    return score;
  }

  template<Color c> int eval_knights(Board& b, EvalInfo& ei) {
    int score = 0;
    int them = c ^ 1;
    U64 pinned = ei.pinned[c];
    KingSafety * kingScores = &ei.ks[c];
    int *sqs = b.sq_of<KNIGHT>(c);
    
    for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs) {      
      if (SquareBB[from] & pinned) score -= PinPenalty[ei.pinners[them][from]][KNIGHT];

      // mobility
      U64 mvs = PseudoAttacksBB(KNIGHT, from);
      U64 mobility = mvs & ei.empty;
      U64 tmp = ei.pe->attacks[them];
      if (tmp) {
	if (tmp & SquareBB[from]) score -= ei.tempoBonus / 2;
	U64 bm = mobility & tmp;
	mobility ^= bm;
      }
      int mcount = count(mobility);
      score += mcount;
      if (mcount <= 1) score -= TrappedPenalty[KNIGHT];
      
      // attack bonuses
      U64 attacks = mvs & ei.pieces[them];
      while (attacks) {
	int to = pop_lsb(attacks);
	int p = b.piece_on(to);
	score += int(AttackBonus[ei.phase][KNIGHT][p]);
      }
      
      // king attacks
      U64 king_threats = PseudoAttacksBB(KNIGHT, from) & KingSafetyBB[them][ei.kingsq[them]];
      if (king_threats) {
	kingScores->attackScore[KNIGHT] += KingAttackBonus[KNIGHT];
	kingScores->numAttackers++;
	kingScores->attackedSquareBB |= king_threats;
      }
    }

    if (ei.do_trace) {
      (c == WHITE ? ei.s.knight_sc[WHITE] = score : ei.s.knight_sc[BLACK] = score);
    }
    return score;
  }

  template<Color c> int eval_bishops(Board& b, EvalInfo& ei) {
    int score = 0;
    int them = (c ^ 1);
    U64 pinned = ei.pinned[c];
    U64 mask = ei.all_pieces;
    bool light_bishop = false;
    bool dark_bishop = false;
    int eks = ei.kingsq[them];
    U64 center_pawns = ei.all_pawns & CenterMaskBB;
    int center_nb = count(center_pawns);
    KingSafety * kingScores = &ei.ks[c];
    int *sqs = b.sq_of<BISHOP>(c);
    
    for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs) {
      if (SquareBB[from] & ColoredSquaresBB[WHITE]) light_bishop = true;
      else dark_bishop = true;
     
      if (SquareBB[from] & pinned) score -= PinPenalty[ei.pinners[them][from]][BISHOP];
      
      U64 mvs = attacks<BISHOP>(mask, from);      
      U64 mobility = mvs & ei.empty;
      U64 tmp = ei.pe->attacks[them];
      if (tmp) {
	if (tmp & SquareBB[from]) score -= ei.tempoBonus / 2;
	U64 bm = mobility & tmp;
	mobility ^= bm;
      }
      int mcount = count(mobility);
      score += mcount;
      if (mcount <= 4) score -= TrappedPenalty[BISHOP];

      U64 attacks = mvs & ei.pieces[them];
      while (attacks) {
	int to = pop_lsb(attacks);
	int p = b.piece_on(to);
	score += int(AttackBonus[ei.phase][BISHOP][p]);
	//U64 weak = SquareBB[to] & ei.weak_enemies[them];
	//if (weak != 0ULL) score += 1; // small bonus.
      }

      // positional
      // note: did try including color penalties for few pawn targets
      if (center_nb <= 2) score += BishopCenterBonus[ei.phase];

      // threats to king 
      U64 king_threats = mvs & PseudoAttacksBB(KING, eks);
      if (king_threats) {
	kingScores->attackScore[BISHOP] += KingAttackBonus[BISHOP];
	kingScores->numAttackers++;
	kingScores->attackedSquareBB |= king_threats;
      }
    }

    // light + dark square bishop bonus
    if (light_bishop && dark_bishop) score += DoubleBishopBonus[ei.phase];

    if (ei.do_trace)
      {
	(c == WHITE ? ei.s.bishop_sc[WHITE] = score : ei.s.bishop_sc[BLACK] = score);
      }
    return int(score);
  }

  template<Color c> int eval_rooks(Board& b, EvalInfo& ei) {    
    int score = 0;
    int them = (c ^ 1);
    U64 pinned = ei.pinned[c];
    U64 mask = ei.all_pieces;
    KingSafety * kingScores = &ei.ks[c];
    int eks = ei.kingsq[them];
    U64 pawns = ei.all_pawns;
    U64 rank7 = (c == WHITE ? RowBB[ROW7] : RowBB[ROW2]);
    int ranks[2]; int files[2]; int idx = 0;

    int *sqs = b.sq_of<ROOK>(c);
    for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs) {      
      // pinned
      U64 mvs = attacks<ROOK>(mask, from);
      if ((SquareBB[from] & pinned)) {
	score -= PinPenalty[ei.pinners[them][from]][ROOK];
      }

      // mobility
      U64 mobility = mvs & ei.empty;
      U64 tmp = ei.pe->attacks[them];
      if (tmp) {
	if (tmp & SquareBB[from]) score -= ei.tempoBonus / 2;
	U64 bm = mobility & tmp;
	mobility ^= bm;
      }
      int mcount = count(mobility);
      if (mobility) score += mcount/2;
      if (mcount <= 4) score -= TrappedPenalty[ROOK];
      
      // attacks
      U64 attacks = mvs & ei.pieces[them];
      while (attacks) {
	int to = pop_lsb(attacks);
	int p = b.piece_on(to);
	score += int(AttackBonus[ei.phase][ROOK][p]);
      }

      // doubled/connected rooks
      int file = COL(from);
      if (idx < 2) {
	ranks[idx] = ROW(from);
	files[idx] = file;
	U64 file_closed = ColBB[file] & pawns;
	if (file_closed == 0ULL) {
	  score += 2 * RookOpenFileBonus[ei.phase];
	  ei.RooksOpenFile[c] = true;
	}
	++idx;
      }
      
      // rank-7
      U64 on7 = SquareBB[from] & rank7;
      if (!empty(on7)) score += 2;
      
      U64 king_threats = mvs & KingSafetyBB[them][eks];
      if (!empty(king_threats)) {
	kingScores->attackScore[ROOK] += KingAttackBonus[ROOK];
	kingScores->numAttackers++;
	kingScores->attackedSquareBB |= king_threats;
	//U64 contact_check = SquareBB[from] & KingSafetyBB[them][eks];
	//if (contact_check != 0ULL) score += 1;
      }

    }
    if (idx >= 2) {
      score += (ranks[0] == ranks[1]) ? 4 : 0;
      score += (files[0] == files[1]) ? 6 : 0;
      score += ((ColBB[files[0]] | ColBB[files[1]]) & b.get_pieces(c, QUEEN)) ? 6 : 0;
    }

    if (ei.do_trace) {
      (c == WHITE ? ei.s.rook_sc[WHITE] = score : ei.s.rook_sc[BLACK] = score);
    }
    return score;
  }

  template<Color c> int eval_queens(Board& b, EvalInfo& ei) {
    int score = 0;    
    int them = (c ^ 1);
    U64 mask = ei.all_pieces;
    int eks = ei.kingsq[them];
    U64 pinned = ei.pinned[c];
    U64 enemy_pawns = ei.pawns[them];
    U64 enemy_knights = b.get_pieces(them, KNIGHT);
    U64 enemy_bishops = b.get_pieces(them, BISHOP);
    U64 enemy_rooks = b.get_pieces(them, ROOK);
    U64 pawns = ei.all_pawns;
    KingSafety * kingScores = &ei.ks[c];
    int *sqs = b.sq_of<QUEEN>(c);
    
    for (int from = *++sqs; from != SQUARE_NONE; from = *++sqs) {
      U64 mvs = (attacks<BISHOP>(mask, from) | attacks<ROOK>(mask, from));

      if (SquareBB[from] & pinned) score -= PinPenalty[ei.pinners[them][from]][QUEEN];

      // note : dropped the mobility check for the queen       
      U64 attacks = mvs & ei.pieces[them];
      while (attacks) {
	int to = pop_lsb(attacks);
	int p = b.piece_on(to);
	score += int(AttackBonus[ei.phase][QUEEN][p]);
      }

      if (b.is_attacked(from, c, them) &
	  (enemy_pawns | enemy_knights | enemy_bishops | enemy_rooks))
	score -= ei.tempoBonus;

      // open file bonus for the queen
      U64 file_closed = ColBB[COL(from)] & pawns;
      if (!file_closed) score += 2;

      // open center bonus
      U64 center_pawns = ei.all_pawns & CenterMaskBB;
      int center_nb = count(center_pawns);
      if (center_nb <= 2) score += BishopCenterBonus[ei.phase];

      U64 king_threats = mvs & KingSafetyBB[them][eks];
      if (king_threats) {
	kingScores->attackScore[QUEEN] += KingAttackBonus[QUEEN];
	kingScores->numAttackers++;
	kingScores->attackedSquareBB |= king_threats;
	//U64 contact_check = SquareBB[from] & KingSafetyBB[them][eks];
	//if (contact_check != 0ULL) score += 2;
      }
    }

    if (ei.do_trace) {
      (c == WHITE ? ei.s.queen_sc[WHITE] = score : ei.s.queen_sc[BLACK] = score);
    }
    return score;
  }

  template<Color c> int eval_kings(Board& b, EvalInfo& ei) {
    int score = 0;
    int from = ei.kingsq[c];
    int them = (c ^ 1);
    U64 their_pawns = ei.pawns[them];
    bool castled = b.is_castled(c);
    U64 mvs = PseudoAttacksBB(KING, from);

    // king safety 
    KingSafety * kingScores = &ei.ks[c];
    U64 king_threats = mvs & KingSafetyBB[them][ei.kingsq[them]];
    if (king_threats) {
      kingScores->attackScore[KING] += KingAttackBonus[KING];
      kingScores->numAttackers++;
      kingScores->attackedSquareBB |= king_threats;
    }
    if (kingScores->numAttackers > 1) {
      int attack_score = 0;
      for (int p = KNIGHT; p <= KING; ++p) attack_score += kingScores->attackScore[p];
      score += (int)kingScores->numAttackers * attack_score * 0.85;
      score += 2 * count(kingScores->attackedSquareBB);

      // does the king have any safety squares
      U64 escapeBB = PseudoAttacksBB(KING, ei.kingsq[them]);			
      U64 unsafeBB = (kingScores->attackedSquareBB) & escapeBB;
      if (unsafeBB != 0ULL) {
	escapeBB ^= unsafeBB;
	escapeBB ^= (escapeBB & ei.pieces[them]);

	if (escapeBB != 0ULL) {
	  int nb_safe = count(escapeBB);
	  if (nb_safe <= 1) score += 3 * attack_score;
	}
	//else if (escapeBB == 0ULL)
	//{
	//	score += (int)kingScores->numAttackers * attack_score * 0.10;
	//	U64 queens = KingVisionBB[them][QUEEN][ei.kingsq[them]] & b.get_pieces(c, QUEEN);
	//	if (queens != 0ULL) score += 14;

	//	U64 rooks = KingVisionBB[them][ROOK][ei.kingsq[them]] & b.get_pieces(c, ROOK);
	//	if (rooks != 0ULL) score += 12;

	//	U64 bishops = KingVisionBB[them][BISHOP][ei.kingsq[them]] & b.get_pieces(c, BISHOP);
	//	if (bishops != 0ULL) score += 4;

	//	U64 knights = KingVisionBB[them][KNIGHT][ei.kingsq[them]] & b.get_pieces(c, KNIGHT);
	//	if (knights != 0ULL) score += 8;

	//	//U64 pawns = KingVisionBB[them][PAWN][ei.kingsq[them]] & b.get_pieces(c, PAWN);
	//	//if (pawns != 0ULL) score += 1;
	//}
      }
    }
    // pawn cover around king -- based on game phase
    if (ei.phase == MIDDLE_GAME) {
      U64 v_mask = KingVisionBB[them][QUEEN][ei.kingsq[them]] &
	//KingSafetyBB[them][ei.kingsq[them]] & 
	their_pawns;
      int nb_blockers = count(v_mask);
      if (nb_blockers < 3 && kingScores->numAttackers > 0) {
	score += 2 * kingScores->numAttackers;
      }

      // extreme case where there are "no" pawns around the king
      U64 pawn_cover = PseudoAttacksBB(KING, ei.kingsq[them]) & their_pawns;
      if (pawn_cover == 0ULL && kingScores->numAttackers > 0) score += 2 * kingScores->numAttackers; // no pawns always good for us

      // extreme case where there are very few king defenders
      U64 piece_cover = KingSafetyBB[them][ei.kingsq[them]] & ei.pieces[them];
      piece_cover ^= ei.pawns[them];
      int nb_covering = count(piece_cover);
      if (nb_covering <= 0 && kingScores->numAttackers > 0) score += 4 * kingScores->numAttackers;
    }

    if (!castled) score -= 4 * CastleBonus[ei.phase] * KingExposureBonus[ei.phase];

    // .. evaluate development and treat our "less active" pieces as dangerous to our king
    //U64 back_rank = (c == WHITE ? RowBB[ROW1] : RowBB[ROW8]);
    //U64 undev_pieces = back_rank & (b.get_pieces(c, KNIGHT) | b.get_pieces(c, BISHOP) | b.get_pieces(c, ROOK) | b.get_pieces(c, QUEEN));
    //if (undev_pieces)
    //{
    //	int nb_undev_pieces = count(undev_pieces);
    //	if (nb_undev_pieces > 2) score -= 2;
    //}

    if (ei.do_trace) {
      (c == WHITE ? ei.s.king_sc[WHITE] = score : ei.s.king_sc[BLACK] = score);
    };
    return score;
  }

  template<Color c> int eval_space(Board& b, EvalInfo& ei) {
    int score = 0;

    // compute the number of squares behind pawns
    U64 pawn_bm = ei.pawns[c];

    // only connected pawns.
    pawn_bm ^= ei.pe->isolatedPawns[c];

    if (pawn_bm == 0ULL) return 0;

    // loop over pawns and count the squares behind them
    U64 space = 0ULL;
    while (pawn_bm) {
      int s = pop_lsb(pawn_bm);
      space |= SpaceBehindBB[c][s];
    }
    space &= (ColBB[COL2] | ColBB[COL3] | ColBB[COL4] | ColBB[COL5]);
    score += count(space);// * SpaceBonus[ei.phase];

    if (ei.do_trace) {
      (c == WHITE ? ei.s.space_sc[WHITE] = score : ei.s.space_sc[BLACK] = score);
    };
    return score;
  }

  template <Color c> int eval_threats(Board &b, EvalInfo& ei) {
    int score = 0;
    int them = c ^ 1;
    U64 knights = b.get_pieces(them, KNIGHT);
    U64 bishops = b.get_pieces(them, BISHOP);
    U64 queens = b.get_pieces(them, QUEEN);
    U64 rooks = b.get_pieces(them, ROOK);

    // are there any discovered check candidates in the position? These would be slider pieces that are pointed at the king,
    // but are blocked (only once) by their own friendly pieces & (friends | enemies)
    /*
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
    */

    // targets
    U64 passed_pawns = ei.pe->passedPawns[them];
    U64 isolated_pawns = ei.pe->isolatedPawns[them];
    U64 backward_pawns = ei.pe->backwardPawns[them];
    U64 doubled_pawns = ei.pe->doubledPawns[them];
    U64 chain_bases = ei.pe->chainBase[them];
    //U64 undefended_pawns = ei.pe->undefended[them];
    //U64 chain_heads = ei.pe->pawnChainTips[them];

    // weak pawn targets for pieces/pawns    
    U64 pawn_targets = (chain_bases | doubled_pawns | backward_pawns | isolated_pawns | passed_pawns);
    if (pawn_targets) {
      while (pawn_targets) {

	int sq = pop_lsb(pawn_targets);
	//int target_value = square_score<c, PAWN>(ei.phase, sq); // ranges from 5-100
	U64 attackers = b.attackers_of(sq);
	if ((attackers & knights) != 0ULL) score += 1;// target_value;
	if ((attackers & bishops) != 0ULL) score += 1;// target_value;
	if ((attackers & queens) != 0ULL) score += 1;// target_value;
	if ((attackers & rooks) != 0ULL) score += 1;// target_value;
      }
    }
    
    // advanced passed pawns
    U64 our_passed_pawns = ei.pe->passedPawns[c];
    if (our_passed_pawns) {
      score += 1;
      while (our_passed_pawns) {
	// pawns close to promotion are almost always a threat
	int from = pop_lsb(our_passed_pawns);
	int infrontSq = (c == WHITE ? from + 8 : from - 8);
	U64 squares_until_promotion = SpaceInFrontBB[c][from];
	U64 blockers = (squares_until_promotion & b.all_pieces());
	if (!blockers) score += 2;
	int sqs_togo = count(squares_until_promotion);
	if (on_board(infrontSq) && ei.phase == END_GAME) {
	  score += (7 - sqs_togo) * 10;// *10;

	  U64 our_rooks = b.get_pieces(c, ROOK);
	  if (our_rooks) {
	    int frook = pop_lsb(our_rooks);
	    int rook_file = COL(frook);
	    if (rook_file == COL(from)) {
	      if (c == WHITE && ROW(frook) < ROW(from)) score += 2;
	      if (c == BLACK && ROW(frook) > ROW(from)) score += 2;
	    }
	  }
	}
	if (sqs_togo <= 3) score += 5;// 20;
      }
    }

    if (ei.do_trace) {
      (c == WHITE ? ei.s.threat_sc[WHITE] = score : ei.s.threat_sc[BLACK] = score);
    };

    return score;
  }

  template<Color c> int eval_center(Board &b, EvalInfo& ei) {
    int score = 0;
    int them = c ^ 1;
    U64 bigCenter = CenterMaskBB;

    U64 pawns = ei.pawns[c];
    U64 knights = b.get_pieces(c, KNIGHT);
    U64 bishops = b.get_pieces(c, BISHOP);
    U64 queens = b.get_pieces(c, QUEEN);
    U64 rooks = b.get_pieces(c, ROOK);

    if (pawns) {
      U64 pawn_bm = pawns & bigCenter;
      if (pawn_bm) score += 2 * count(pawn_bm);// add weight for pawn attacks - helps opening play
    }
    if (knights) {
      U64 knight_bm = knights & bigCenter;
      if (knight_bm) score += 1;
    }
    if (bishops) {
      U64 bish_bm = bishops & bigCenter;
      if (bish_bm) score += 1;
    }

    // minor/major in infront of center pawn penalty
    if (ei.phase == MIDDLE_GAME) {
      U64 target_sqs = (c == WHITE ? SquareBB[E3] | SquareBB[D3] : SquareBB[E6] | SquareBB[D6]);
      U64 pawns_2nd_rank = (c == WHITE ? pawns & (SquareBB[E2] | SquareBB[D2]) : pawns & (SquareBB[E7] | SquareBB[D7]));
      if (pawns_2nd_rank != 0ULL) {
	U64 bad_positioned_pieces = (bishops | queens | rooks) & target_sqs;
	if (bad_positioned_pieces != 0ULL) score -= 16; // it really hurts the middlegame .. 
      }
    }

    // central weaknesses
    U64 doubled_pawns = ei.pe->doubledPawns[them];
    U64 weaknesses = doubled_pawns & bigCenter;// | undefended_pawns) & bigCenter;
    if (ei.phase == MIDDLE_GAME && weaknesses != 0ULL) {
      score += 1;
    }

    if (ei.do_trace) {
      (c == WHITE ? ei.s.center_sc[WHITE] = score : ei.s.center_sc[BLACK] = score);
    };
    return score;
  }

  template<Color c> int eval_pawn_levers(Board& b, EvalInfo& ei)
  {
    int score = 0;
    int them = c ^ 1;

    // if we have the move adjust the score a little
    // since we can initiate the capture
    //if (b.whos_move() == c) score += 1;

    U64 pawn_lever_attacks = ei.pe->attacks[c] & ei.pawns[them];
    while (pawn_lever_attacks) {
      int to = pop_lsb(pawn_lever_attacks);
      score += PawnLeverScore[to];
    }
    return score;
  }

  void traceout(EvalInfo& ei) {
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
    printf("| Center      |\t %d \t|\t %d \t|\t %d \t|\n", ei.s.center_sc[WHITE], ei.s.center_sc[BLACK], ei.s.center_sc[WHITE] - ei.s.center_sc[BLACK]);
    printf("| Threats     |\t %d \t|\t %d \t|\t %d \t|\n", ei.s.threat_sc[WHITE], ei.s.threat_sc[BLACK], ei.s.threat_sc[WHITE] - ei.s.threat_sc[BLACK]);
    printf("| Eval time   |\t -- \t|\t -- \t|\t %d \t|\n", ei.s.time);
  }
};

namespace Eval {
  int evaluate(Board& b) { return eval(b); }
};
