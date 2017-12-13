#include <cstring>

#include "mctree.h"
#include "uci.h"
#include "search.h"
#include "moveselect.h"

//------------------------------------------------------
// node utilities
//------------------------------------------------------
MCNode::MCNode() : visits(0), score(0), parent(0) {
  child.clear();
}

MCNode::MCNode(Board& b, MCNode * p) : visits(0), score(0), parent(p) {  
  child.clear();
  for (MoveGenerator mvs(b); !mvs.end(); ++mvs) {
    MCNode n(this, mvs.move());
    child.push_back(n);
  }
}

MCNode::MCNode(MCNode * p, U16 m) : visits(0), score(0), parent(p) {
  child.clear();
  ML.m = m; ML.v = 0;
}

MCNode::~MCNode() { }

//------------------------------------------------------
// monte carlo tree search methods
//------------------------------------------------------
MCTree::MCTree(Board& b) : tree(0), rand(0) {
  tree = new MCNode(b);
  rand = new MT19937(0, 1);
  this->b = &b;
}

MCTree::~MCTree() {
  if (tree) { delete tree; tree = 0; }
  if (rand) { delete rand; rand = 0; }
}

MCNode * MCTree::select(MCNode * n, Board * brd) {
  BoardData bd;
  while (has_child(n)) {
    n = pick_child(n, *brd);
    n->do_move(bd, (*brd));
    ++reduction;
  }
  return n;
}

void MCTree::add_children(MCNode * n, Board * brd) {
  MCNode nn(*brd);
  for (unsigned int j=0; j<nn.child.size(); ++j) {
    nn.child[j].parent = n;
    n->child.push_back(nn.child[j]);
  }
}

bool MCTree::has_child(MCNode * n) {  
  return n->child.size() > 0;
}

MCNode * MCTree::pick_child(MCNode* n, Board& b) {
  int mvs = n->child.size();
  double max = NINF;
  double C = 1;
  int id = -1; int N = n->visits;
  int tried = 0;
  
  double p = rand->next();
  if (p <= .7) {
    for (int j=0; j<mvs; ++j) {
      int nn = n->child[j].visits;
      double sc = (double) n->child[j].score;
      double v = (double) n->child[j].visits;
      if (v > 0) {
	double r = sc / v;
	double UCT = r + C * sqrt((double)(2.0 * log(N) / (double) nn) );
	if (UCT > max) { max = UCT; id = j; }
	++tried;
      }
    }
  }
  if (id < 0) id = (int)(rand->next() * mvs); // real monte carlo ;)
  return &n->child[id];
}

float MCTree::expand(MCNode * n, Board * brd, int depth) {
  if (n->child.size() <= 0) { 
    add_children(n, brd);
  }
  
  float s = minimax(*brd, depth);

  n->visits++; // auto-adds node to tree
  n->score = s;
  return s;
}

void MCTree::update(MCNode * n, float score) {
  while (n->has_parent()) {
    n = n->parent;
    score = -score;
    n->visits++; n->score += score;
  }
}

bool MCTree::search(int depth) {
  int bound = bootstrap(*b);
  for (unsigned int j = 0; j < RootMoves.size(); ++j) {
    U16 mv = RootMoves[j].m;
    printf("info depth 4 currmove %s score %d\n", 
           UCI::move_to_string(mv).c_str(), 
           RootMoves[j].v);    
  }
  printf("..initial eval = %d\n", bound);
  
  /*
  int trials = 0; reduction = 0;
  while(trials < 300 || has_ties()) {
    Board brd(*b);
    MCNode * n = select(tree, &brd);
    float score = expand(n, &brd, depth);
    update(n, score);
    ++trials;
  }
  print_pv();
  printf("..mc search took %d trials to converge\n", trials);
  */
  return true;
}

bool MCTree::has_ties() {
  float max = NINF;
  int maxcount = 0;
  MCNode * n = tree;
  int sz = n->child.size();
  
  for(int j=0; j<sz; ++j) {      
    if (n->child[j].visits > 0) {
      float p = (float)((float)n->child[j].score / (float)n->child[j].visits);
      if (p > max) max = p;
    }
  }
  if (max == NINF) return true;
  
  for (int j=0; j<sz; ++j) {    
    if (n->child[j].visits > 0) {      
      float p = (float)((float)n->child[j].score / (float)n->child[j].visits);
      if (max == p) ++maxcount;
    }
  }
  return maxcount > 1;
}

void MCTree::print_pv() {
  std::vector<U16> root_moves;
  MCNode * n = tree;
  float max = NINF; int id = -1;
  
  while (has_child(n)) {
    
    max = NINF; id = -1;
    
    for(unsigned int j=0; j<n->child.size(); ++j) {      
      if (n->child[j].visits > 0) {
	float p = (float)((float)n->child[j].score / (float)n->child[j].visits);
	if (p > max) { max = p; id = j; }
	U16 mv = n->move(j);
	printf("idx(%d), move(%s), visits(%3.3f), wins(%3.3f), p(%3.3f)\n", j,
	       UCI::move_to_string(mv).c_str(),
	       n->child[j].visits, 
	       n->child[j].score,
	       p);	
      }
    }
    
    printf("  max(%3.3f), id(%d)\n\n", max, id);
    if (id >= 0) {
      root_moves.push_back(n->move(id));
      n = &n->child[id];
    }
    else break;
  }
  std::string pv = "";
  if (root_moves.size() <= 0) pv = "(none)";
  for (unsigned int j = 0; j < root_moves.size(); ++j) {
    pv += UCI::move_to_string(root_moves[j]) + " ";      
  }
  printf("info (monte-carlo search) pv ");
  std::cout << pv << std::endl;
}

int MCTree::bootstrap(Board& b) {
  return Search::mc_minimax(b, 6);
}

float MCTree::minimax(Board& b, int depth) {
  int newdepth = depth - reduction;
  newdepth = (newdepth < 1 ? 2 : newdepth);
  int eval = -Search::mc_minimax(b, newdepth);
  return (eval <= -66 ? -1 : eval <= 66 ? 0 : 1);
  //return (float)((float)eval / (float)9999);
}
