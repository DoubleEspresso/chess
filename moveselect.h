#pragma once
#ifndef HEDWIG_MOVESELECT_H
#define HEDWIG_MOVESELECT_H
#include <cassert>
#include <string.h>

#include "definitions.h"
#include "move.h"
#include "board.h"
#include "search.h"
//struct Node;

struct MoveStats
{
  int history[2][SQUARES][SQUARES];
  U16 countermoves[SQUARES][SQUARES];

  void update(Board& b, U16& m, U16& last, Node* stack, int d, int eval, U16* quiets);
  
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

  }
};

enum SelectPhase { PHASE_TT, PHASE_CAPTURE_GOOD, PHASE_KILLER1, PHASE_KILLER2,
		    PHASE_KILLER3, PHASE_KILLER4, PHASE_CAPTURE_BAD, PHASE_QUIET, PHASE_END };

class MoveSelect
{
  int c_sz, stored_csz;
  int q_sz, stored_qsz;
  int select_phase;
  MoveList captures[MAX_MOVES];
  MoveList quiets[MAX_MOVES];
  //U16 killers[4];
  MoveStats * statistics;
  SearchType type;
  bool givesCheck;

 public:
 MoveSelect(MoveStats& stats, SearchType t) : 
  c_sz(0), stored_csz(0), q_sz(0), 
    stored_qsz(0), select_phase(PHASE_TT), statistics(0), type(t), givesCheck(false)
  {   
    // fixme...
    for (int j = 0; j<MAX_MOVES; ++j)
      {
	captures[j].score = NINF - 1; captures[j].m = MOVE_NONE;
	quiets[j].score = NINF - 1; quiets[j].m = MOVE_NONE;
      }   
    statistics = &stats;
  }  
  
 MoveSelect(MoveStats& stats, SearchType t, bool checksKing) : 
  c_sz(0), stored_csz(0), q_sz(0), 
    stored_qsz(0), select_phase(PHASE_TT), statistics(0), type(t), givesCheck(checksKing)
  {   
    // fixme...
    for (int j = 0; j<MAX_MOVES; ++j)
      {
	captures[j].score = NINF - 1; captures[j].m = MOVE_NONE;
	quiets[j].score = NINF - 1; quiets[j].m = MOVE_NONE;
      }   
    statistics = &stats;
  }  


  ~MoveSelect() {};
  
  void sort(MoveList * ml, int length);
  bool nextmove(Board &b, Node * stack, U16& ttm, U16& out, bool split);

  int phase() { return select_phase; }
  // dbg
  MoveList * get_quites() { return quiets; }
  int qsize() { return stored_qsz; }
  int csize() { return stored_csz; }
  MoveList * get_captures() { return captures; }
  void print_list();
  void load_and_sort(MoveGenerator& mvs, Board& b, U16& ttm, Node * stack, MoveType movetype);
};

#endif
