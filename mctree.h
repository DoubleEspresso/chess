#pragma once
#ifndef SBC_MCTREE_H
#define SBC_MCTREE_H

#include <vector>
#include <algorithm>
#include <random>
#include <string>
#include <memory>

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
  float visits, score;
  MoveList ML;
  std::vector<MCNode> child;    
  std::shared_ptr<MCNode> parent; // never allocated

  MCNode();
  MCNode(MCNode * p, U16 move);
  ~MCNode() { child.clear(); }
  
  inline void do_move(BoardData& bd, Board& b);
  inline bool has_parent();
  inline U16 childmove(int j);
};

inline void MCNode::do_move(BoardData& bd, Board& b) {
  b.do_move(bd, ML.m);
}

inline bool MCNode::has_parent() {
  return parent != NULL;
}

inline U16 MCNode::childmove(int j) {
  return child[j].ML.m;
}

class MCTree {
  std::shared_ptr<MCNode> tree;
  std::shared_ptr<MT19937> rand;
  
 public:
 MCTree() : tree(new MCNode()), rand(new MT19937(0,1)) {}
  
  bool has_child(MCNode& n);
  void print_pv();
  bool search(Board &b);  
  MCNode * pick_child(MCNode*& n, Board& b);
  int uct_select(MCNode* n);
  MCNode * pick_capture(MCNode*& n, Board& b);
  void add_children(MCNode*& nn, Board& b);
  float rollout(MCNode*& nn, Board& b);
  float resolve(MCNode*& nn, Board& b);
  void update(MCNode*& nn, float score); 
};


#endif
