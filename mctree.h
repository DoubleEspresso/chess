#pragma once
#ifndef SBC_MCTREE_H
#define SBC_MCTREE_H

#include <iostream>
#include <stdio.h>
#include <memory>
#include <vector>
#include <cstring>
#include <string>
#include <random>
#include <algorithm>

#include "definitions.h"
#include "globals.h"
#include "utils.h"
#include "board.h"
#include "move.h"
#include "evaluate.h"
#include "uci.h"

// std::make_unique is part of c++14
template<typename T, typename... Args>
  std::unique_ptr<T> make_unique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

class MT19937 {
  std::unique_ptr<std::mt19937> engine;
  std::unique_ptr<std::uniform_real_distribution<double>> dist;
  
 public:
 MT19937() : MT19937(0,1) {}
  MT19937(double min, double max) {
    auto const seed = std::random_device()();
    engine = make_unique<std::mt19937>(seed);
    dist = make_unique<std::uniform_real_distribution<double>>(min, max);
  }
  
  double next() {return (*dist)(*engine); }
};


class MCNode : public std::enable_shared_from_this<MCNode> {
  MoveList ML;
  std::weak_ptr<MCNode> parent;
  std::vector< std::shared_ptr<MCNode> > children;
  
 public:
  MCNode();
  MCNode(std::shared_ptr<MCNode> p, U16 m);
  MCNode(const MCNode& other) = delete;
  MCNode& operator=(const MCNode& other) = delete;  
  float visits, score;
  
  inline bool has_parent() { return !parent.expired(); }
  inline bool has_children() { return children.size() > 0; }
  inline size_t num_children() { return children.size(); }
  inline U16 child_move(int j) { return children[j]->get_move(); }
  inline U16 get_move() { return ML.m; }
  void add_children(Board& b);
  inline void add_child(std::shared_ptr<MCNode> c) { children.push_back(c); }
  inline std::shared_ptr<MCNode> get_parent() { return parent.lock(); }
  inline std::shared_ptr<MCNode> get_child(size_t j) { return children[j]; }
};


class MCTree {
  std::shared_ptr<MCNode> root, curr; 
  MT19937 rand;
  
  int uct_select();
  int pick_child(Board& b);
  int pick_capture(Board& b);
  float rollout(Board& b);
  float resolve(Board& b);
  void traceback(float score);
  inline double get_rand() { return rand.next(); }
    
 public:
  MCTree();
  MCTree(const MCTree& other) = delete;
  MCTree& operator=(const MCTree& other) = delete;
  
  bool search(Board& b);
  void print_pv();
};

#endif
