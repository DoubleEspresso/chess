#pragma once
#ifndef SBC_MCTREE_H
#define SBC_MCTREE_H

#include <vector>
#include <algorithm>
#include <random>
#include <string>

#include "definitions.h"
#include "globals.h"
#include "utils.h"
#include "board.h"
#include "move.h"

class MT19937 {
  std::mt19937 * engine;
  std::uniform_real_distribution<double> * dist;
 public:
  MT19937(double min, double max) {
    auto const seed = std::random_device()();
    engine = new std::mt19937(seed);
    dist = new std::uniform_real_distribution<double>(min, max);
  }
  ~MT19937() {
    if (dist) { delete dist; dist = 0; }
    if (engine) { delete engine; engine = 0; }      
  }
  double next() { return (*dist)(*engine); }
};

struct MCNode {
  MCNode();
  MCNode(Board& b, MCNode * p = NULL);
  MCNode(const U16& move);
  ~MCNode();
  
  int visits, score;
  U16& move;
  std::vector<MCNode> child;    
  MCNode * parent; // never allocated
  
  inline void do_move(Board& b);
};

inline void MCNode::do_move(BoardData& bd, Board& b) {
  b.do_move(bd, move);
}


class MCTree {
  MCNode * tree;
  MT19937 * rand;
  Board * b;
  std::vector<U16> RootMoves;  
    
 public:
  MCTree(Board& b);
  ~MCTree();
  
  Node * select();
  Node * pick_child(Node * n);
  bool expand(Node& n);
  void update(Node& n); 
  void simulate(Node& n);     
  void print_pv();
  bool search(int depth);
};


#endif
