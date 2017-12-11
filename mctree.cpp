#include "mctree.h"
#include "uci.h"

//------------------------------------------------------
// node utilities
//------------------------------------------------------
MCNode::MCNode() :
  visits(0), score(0), move(0), parent(0)
{
  child.clear();
}

MCNode::MCNode(Board& b, MCNode * p) :
  visits(0), score(0), move(0), parent(p)
{  
  child.clear();
  for (MoveGenerator mvs(b); !mvs.end(); ++mvs) {
    MCNode n(this, mvs.move());
    child.push_back(n);
  }
}

MCNode::MCNode(MCNode * p, U16 m) :
  visits(0), score(0), move(m), parent(p)
{
  child.clear();
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
    n = pick_child(n);
    n->do_move(bd, (*brd));
  }
  return n;
}

void MCTree::add_children(MCNode * n, Board * brd) {
  MCNode nn(*brd);
  for (unsigned int j=0; j<nn.child.size(); ++j) {
    n->child.push_back(nn.child[j]);
  }
}

bool MCTree::has_child(MCNode * n) {  
  for (unsigned int j=0; j<n->child.size(); ++j) {
    MCNode sn = n->child[j];
    if (sn.visits > 0) return true;
  } 
  //return false; 
  return n->child.size() > 0;
}

MCNode * MCTree::pick_child(MCNode* n) {
  // TODO : implement UTC style algorithm 
  // for now..simple scheme to select a random move with a prior move histogram
  // 1. generate a random number in [0,1] uniformly --> picks a move idx
  // 2. p2 = generate a 2nd random number in [0,1]
  // 3. if wins/tries for selected mv is >= p2, accept idx @ step 1 else retry
  // 4. if after N tries, there is no valid move, pick a random idx

  int trials = 6; int mvs = n->child.size();
  MCNode * sn;
  for (int j=0; j < trials; ++j) {
    int id = (int)(rand->next() * mvs);
    double p = rand->next();
    sn = &n->child[id];
    sn->parent = n;
    double score = -1;
    if (sn->visits > 0) score = (double)((double)sn->score / (double)sn->visits );
    if (score >= p) break;
  }
  return sn;
}

bool MCTree::expand(MCNode * n, Board * brd) {  
  if (n->child.size() <= 0) { 
    add_children(n, brd);
  }
  double r = rand->next();
  int score = (r <= 0.33 ? -1 : r <= 0.66 ? 0 : 1); // simulate minimax search
  n->visits++; // auto-adds node to tree
  n->score += score;
  return true;
}

void MCTree::update(MCNode * n) {
  RootMoves.clear();
  MCNode * p = n->parent;
  RootMoves.push_back(n->move);
  p->visits++; p->score += n->score;

  while (p->has_parent()) {
    n = p;
    p = n->parent;
    RootMoves.push_back(n->move);
    p->visits++; p->score += n->score;
  }
}

bool MCTree::search(int depth) {
  for (int N= 0; N<20000; ++N) {
    Board brd(*b);
    MCNode * n = select(tree, &brd);
    expand(n, &brd);
    update(n);
  }
  print_pv();
  return true;
}

void MCTree::print_pv() {
  printf("..\n");
  std::string pv = "";
  if (RootMoves.size() <= 0) pv = "(none)";
  for (unsigned int j = RootMoves.size() - 1; j > 0; --j) {
    pv += UCI::move_to_string(RootMoves[j]) + " ";      
  }
  printf("info (monte-carlo search) pv ");
  std::cout << pv << std::endl;
}


