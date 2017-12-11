
#include "mctree.h"
#include "uci.h"


//------------------------------------------------------
// node utilities
//------------------------------------------------------
MCNode::MCNode() :
  visits(0), score(0), move(MOVE_NONE), parent(0)
{
  child.clear();
}

MCNode::MCNode(Board& b, MCNode * p) :
  visits(0), score(0), move(MOVE_NONE), parent(0)
{  
  child.clear();
  for (MoveGenerator mvs(b); !mvs.end(); ++mvs) {
    Node n(mvs.move());
    child.push_back(this, n);
  }
}

MCNode::MCNode(const Node * p, const U16& m) :
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

Node * MCTree::select() {
  Node * n = pick_child(tree); // root
  BoardData bd;
  while (has_child(n)) {
    n->do_move(bd, b);
    n = pick_child(n);
  }
  return n;
}

bool MCTree::has_child(Node * n) {
  for (int j=0; j<n->child.size(); ++j) {
    Node sn = n->child[j];
    if (sn.visits > 0) return true;
  }
  return false;
}

Node * MCTree::pick_child(Node* n) {
  // TODO : implement UTC style algorithm 
  // for now..simple scheme to select a random move with a prior move histogram
  // 1. generate a random number in [0,1] uniformly --> picks a move idx
  // 2. p2 = generate a 2nd random number in [0,1]
  // 3. if wins/tries for selected mv is >= p2, accept idx @ step 1 else retry
  // 4. if after N tries, there is no valid move, pick a random idx
  int trials = 6;
  int mvs = n->child.size();
  Node * sn;
  for (int j=0; j < trials; ++j) {
    int id = (int)(rand->next() * mvs);
    double p = rand->next();
    sn = &n->child[id];
    double score = -1;
    if (sn->visits > 0) score = (double)((double)sn->score / (double)sn->visits );
    if (score >= p) break;
  }
  return sn;
}

bool MCTree::expand(Node& n) {
  n.visits++;
  double r = rand->next();
}

void MCTree::simulate() {
  MCNode * pn = curr->parent;
  int idx = pn->sidx;
  printf("..parent mv index = %d\n", idx);
  MoveList * m = &(pn->mg->get(idx));
  m->tries++;
  bool win = (rand->next() >= rand->next());
  m->wins += (win ? 1 : 0);
}

bool MCTree::select(MCNode * n) {
  if (n == NULL || n->mg == NULL || n->mg->size() <= 0) return false;

  MoveGenerator * g = n->mg;
  int sz = g->size();
  printf("..legal mv-size = %d\n", sz);
  int sidx = -1;
  
  if (!n->has_child() && n->mg != NULL) { // no prior info about these moves
    
    sidx = (int)(rand->next() * sz); // select one randomly (!)
    printf("..no child nodes, selected random idx=%d\n", sidx);
    
    if (sidx > sz) sidx = sz - 1; // sanity check on random selection
    if (sidx < 0) sidx = 0;    
    
    U16 smove = (g->get(sidx)).m;
    
    BoardData bd;
    n->sidx = sidx;
    Board * nb = new Board(*(n->b));
    nb->do_move(bd, smove);
    nb->print();
    MCNode * nn = new MCNode(*(nb), n); // note: updates parent pointer + gens new move list
    n->child.push_back(*nn);
    curr = nn;
    simulate();
    select(stack);
  }
  else { // we have prior win/try stats at this node
    
    std::vector<float> probs;
    for (int j=0; j < sz; ++j) {
      MoveList ml = g->get(j);
      if (ml.tries > 0) probs.push_back((float)((float)ml.wins / (float)ml.tries));
    }
    printf("..dbg - prior win/try info .. probs.size()=%u\n", probs.size());

    // note: we have a selected move, but we may not have tried it yet..(ntries = 0)
    // in this case, we have to treat it as a new node without children, and make the move    
    // create a node, and add it to the list (then return true, do not recurse..)

    MoveList ml = g->get(sidx);
    U16 smove = ml.m;
    bool seen_before = ml.tries > 0;    
    if (seen_before) {
      MCNode * nn = &(n->child[sidx]);
      n->sidx = sidx;
      nn->parent = n;
      curr = nn;
      select(curr); // recurse to leaf node      
    }
    else { // new move!
      BoardData bd;
      n->sidx = sidx;
      Board * nb = new Board(*(n->b));
      nb->do_move(bd, smove);
      nb->print();
      MCNode * nn = new MCNode(*nb, n); // note: updates parent pointer + gens new move list
      n->child.push_back(*nn);
      curr = nn;
      //select(curr);
      simulate();
      select(curr);
    }    
  }
  printf("..dbg finish select(node)\n");
  return true;
}

bool MCTree::search(int depth) {
  for (int d=0; d<depth; ++d) {
    //curr = stack;
    select(stack);
    //simulate();
  }
  print_pv();
  return true;
}

void MCTree::print_pv() {
    MCNode * n = stack;
    std::string pv = "";
    while (n->has_child()) {
      MoveGenerator * g = n->mg;
      int sz = g->size();
      double max = -1; int sidx = 0;
      for (int j=0; j < sz; ++j) {
	MoveList ml = g->get(j);
	float wr = -1;
	if (ml.tries > 0) //wr = (float)((float)ml.wins / (float)ml.tries);
	{
	  //if (wr > max) { max = wr; sidx = j; }
	  if (ml.tries > max) { max = ml.tries; sidx = j; } // note: many moves have wr = 0 (!)
	}
	printf("..mv=%s, max_idx=%d, tries=%3.1f\n", 
	       UCI::move_to_string(ml.m).c_str(), sidx, max);
      }
      U16 bestmove = g->get(sidx).m;
      n = &(n->child[sidx]);
      pv += UCI::move_to_string(bestmove) + " ";      
    }
    printf("info (monte-carlo search) pv ");
    std::cout << pv << std::endl;
}


