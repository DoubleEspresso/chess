#pragma once
#ifndef HEDWIG_MOVESELECT_H
#define HEDWIG_MOVESELECT_H
#include <cassert>
#include <string.h>

#include "definitions.h"
#include "move.h"


struct Node;

struct MoveStats
{
  int history[2][SQUARES][SQUARES];
  U16 countermoves[SQUARES][SQUARES];
  U16 killers[2];
  U16 refutation;
  void update(U16& m, U16& last, Node* stack, int d, int c, U16* quiets);
  
  int score(U16& m, int c)
  {
    int f = get_from(m);
    int t = get_to(m);
    return history[c][f][t];
  }
  void init()
  {
    for (int c = 0; c<2; ++c)
      for (int i = 0; i < SQUARES; ++i)
	for (int j = 0; j < SQUARES; ++j)
	  history[c][i][j] = NINF - 1;

    for (int s1 =0; s1 < SQUARES; ++s1)
      for (int s2 =0; s2 < SQUARES; ++s2)
	countermoves[s1][s2] = MOVE_NONE;

    memset(killers, 0, 2 * sizeof(U16));
    refutation = MOVE_NONE;
  }
};

enum SelectPhase { PHASE_TT, PHASE_CAPTURE_GOOD, PHASE_KILLER1, PHASE_KILLER2, PHASE_CAPTURE_BAD,  PHASE_QUIET, PHASE_END };

class MoveSelect
{
 public:
 MoveSelect(SearchType t) : c_sz(0), stored_csz(0), q_sz(0), stored_qsz(0), use_tt(0), ttmv(MOVE_NONE), select_phase(PHASE_TT), statistics(0), type(t)
    {
      // slow init ..
      for (int j = 0; j<MAX_MOVES; ++j)
	{
	  captures[j].score = NINF - 1; captures[j].m = MOVE_NONE;
	  quiets[j].score = NINF - 1; quiets[j].m = MOVE_NONE;
	}
      killers[0] = killers[1] = MOVE_NONE;
    }
  
  ~MoveSelect() {};
  
  void load(MoveGenerator& mvs, Board& b, U16 tt_mv, MoveStats& stats, Node * stack);
  void sort(MoveList * ml, int length);
  bool nextmove(Node& node, U16& move, bool split);
  //bool threadsafe_nextmove(Board& b, U16& out);
  int phase() { return select_phase; }
  // dbg
  MoveList * get_quites() { return quiets; }
  int qsize() { return stored_qsz; }
  int csize() { return stored_csz; }
  MoveList * get_captures() { return captures; }
  void print_list();
  
 private:
  int c_sz, stored_csz;
  int q_sz, stored_qsz;
  bool use_tt;
  U16 ttmv;
  int select_phase;
  MoveList captures[MAX_MOVES];
  MoveList quiets[MAX_MOVES];
  U16 killers[2];
  MoveStats * statistics;
  SearchType type;
};

#endif
