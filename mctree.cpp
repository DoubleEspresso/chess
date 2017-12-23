#include <cstring>

#include "mctree.h"
#include "uci.h"
#include "search.h"
#include "moveselect.h"
#include "evaluate.h"

//------------------------------------------------------
// node utilities
//------------------------------------------------------
MCNode::MCNode() : visits(0), score(0), parent(NULL) {
  ML.m = MOVE_NONE; ML.v = 0;
  child.clear();
}

MCNode::MCNode(MCNode * p, U16 m) : visits(0), score(0), parent(p) {
  child.clear();
  ML.m = m; ML.v = 0;
}

MCNode::~MCNode() { }

//------------------------------------------------------
// monte carlo tree search methods
//------------------------------------------------------
MCTree::MCTree() : tree(0), rand(0) {
  tree = new MCNode();
  rand = new MT19937(0, 1);
}

MCTree::~MCTree() {
  if (tree) { delete tree; tree = 0; }
  if (rand) { delete rand; rand = 0; }
}

void MCTree::add_children(MCNode*& n, Board& B) {  
  MoveGenerator mvs(B); // all legal mvs
  for (int j=0; !mvs.end(); ++mvs, ++j) {    
    MCNode mcn(n, mvs.move());
    n->child.push_back(mcn);
  }
}

bool MCTree::has_child(MCNode& n) {  
  return n.child.size() > 0;
}

int MCTree::uct_select(MCNode * n) {
  double max = NINF;
  int id = -1;
  int N = n->visits;
  double C = 1;
  int mvs = n->child.size();

  if (mvs <= 0) return id;
  
  double p = rand->next();  
  if (p <= 0.99) {  
    for (int j=0; j<mvs; ++j) {
      int nn = n->child[j].visits;
      double sc = (double) n->child[j].score;
      double v = (double) n->child[j].visits;
      if (v > 0) {
        double r = sc / v;
        double UCT = r + C * sqrt((double)(2.0 * log(N) / (double) nn) );
        if (UCT > max) { max = UCT; id = j; }
      }
    }
  }
  
  if (id < 0) id = (int)(rand->next() * mvs); // fixme
  
  return id;
}

MCNode * MCTree::pick_child(MCNode*& n, Board& b) {
  
  if (!has_child(*n)) add_children(n, b);  
  int id = uct_select(n);
  return (id < 0 ? NULL : &(n->child[id]));
}


void MCTree::update(MCNode*& n, float score) {
  while (n->has_parent()) {
    n = n->parent;
    score = 1 - score;
    n->visits++; n->score += score;
  }
}

bool MCTree::search(Board& b) {
  
  int trials = 0;

  while(trials < 20000) {
    Board B(b); MCNode * n = tree;
    float score = rollout(n, B);
    update(n, score);
    ++trials;
  }
  print_pv();

  return true;
}

float MCTree::rollout(MCNode*& n, Board& B) {
  float eval = NINF;
  bool stop = false;
  int moves_searched = 0;
  BoardData bd;
  bool do_resolve = true;
  
  while (!stop) {
    MCNode * tmp = pick_child(n, B);
    
    if (tmp == NULL) {
      if (B.in_check()) eval = 0;
      else eval = 0.5;
      do_resolve = false;
      break;
    }

    n = tmp;    
    U16 move = n->ML.m;
    bool hasmove = move != MOVE_NONE;
    
    if (!hasmove && B.in_check()) {
      stop = true;
      do_resolve = false;
      eval = 0; 
      break;
    }
    else if (!hasmove && B.is_draw(0)) {
      stop = true;
      do_resolve = false;
      eval = 0.5;
      break;
    }
    else {
      B.do_move(bd, move);
      ++moves_searched;
    }
    
    if (moves_searched == 4) stop = true;
  }
  
  if (do_resolve) {
    eval = resolve(n, B);
  }
  
  n->visits++;
  n->score = eval;
  
  return eval;
}

float MCTree::resolve(MCNode*& n, Board& B) {
  float eval = NINF;
  bool stop = false;
  int moves_searched = 0;
  int cutoff = 90;
  BoardData bd;
    
  while (!stop) {
    
    MCNode * tmp = pick_capture(n, B);
    if (tmp == NULL) { stop = true; break; } // no captures

    n = tmp;    
    U16 move = n->ML.m;
    bool hasmove = move != MOVE_NONE;
    
    if (!hasmove && B.in_check()) {
      stop = true;
      eval = 0; // mate condition
    }
    else if (B.is_repition_draw()) { // stalemate checked in rollout
      stop = true;
      eval = 0.5;
    }
    else if (!hasmove) {
      stop = true;
    }
    else {
      B.do_move(bd, move);
      ++moves_searched;
    }
  }

  eval = Eval::evaluate(B);  
  eval = (eval <= -cutoff ? 0 : eval <= cutoff ? 0.5 : 1);
  return eval;
}


MCNode * MCTree::pick_capture(MCNode*& n, Board& b) {
  
  if (!has_child(*n)) add_children(n, b);
  
  int mvs = n->child.size();
  
  // dummy capture node
  MCNode caps;
  for (int j=0; j<mvs; ++j) {
    U16 mv = n->childmove(j);
    int type = int((mv & 0xf000) >> 12);
    if (type == CAPTURE) caps.child.push_back(*n); // some are checks!    
  }

  int id = uct_select(&caps);
  
  
  if (id < 0) return NULL;  // no captures  
  
  for (int j=0; j<mvs; ++j) {    
    if (n->childmove(j) == caps.childmove(id)) { id = j; break; }     
  }

  if (id < 0) return NULL; // after search (sanity check)
  
  return &(n->child[id]);
}

void MCTree::print_pv() {
  std::vector<U16> root_moves;
  MCNode * n = tree;
  float max = NINF; int id = -1;

  while (has_child(*n)) {
    
    max = NINF; id = -1;
    
    for(unsigned int j=0; j<n->child.size(); ++j) {
      
      if (n->child[j].visits > 0) {
        float p = (float)((float)n->child[j].score / (float)n->child[j].visits);
        if (p > max) { max = p; id = j; }
        
        U16 mv = n->childmove(j);	
        printf("idx(%d), move(%s), visits(%3.3f), wins(%3.3f), p(%3.3f)\n", j,
               UCI::move_to_string(mv).c_str(),
               n->child[j].visits, 
               n->child[j].score,
               p);        
      }
    }    
    if (id >= 0) {
      root_moves.push_back(n->childmove(id));
      n = &n->child[id];
    }
    else break;
  }
  std::string pv = "";
  if (root_moves.size() <= 0) { printf("pv (none)\n"); return; }
  for (unsigned int j = 0; j < root_moves.size(); ++j) {
    pv += UCI::move_to_string(root_moves[j]) + " ";      
  }
  
  std::cout << std::endl;
  std::cout << "info pv " << pv << std::endl;
  std::cout << "bestmove " << UCI::move_to_string(root_moves[0]) << std::endl;
}
