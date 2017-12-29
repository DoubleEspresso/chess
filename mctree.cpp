#include "mctree.h"

//------------------------------------------------------
// node methods
//------------------------------------------------------
MCNode::MCNode() : parent(std::weak_ptr<MCNode>()), visits(0), score(0) {
  children.clear();
  ML.m = MOVE_NONE; ML.v = 0;
}

MCNode::MCNode(std::shared_ptr<MCNode> p, U16 m) : parent(p), visits(0), score(0) {
  children.clear();
  ML.m = m; ML.v = 0;
}

void MCNode::add_children(Board& b) {
  auto p = shared_from_this();

  MoveGenerator mvs(b); // all legal mvs
  
  for (; !mvs.end(); ++mvs) {
    auto node = std::make_shared<MCNode>(p, mvs.move());
    children.push_back(node);    
  }
}


//------------------------------------------------------
// monte carlo tree search methods
//------------------------------------------------------

int MCTree::uct_select() {
  double max = NINF;
  int id = -1;
  int N = curr->visits;
  double C = 1;
  int mvs = curr->num_children();

  if (mvs <= 0) return id; // -1
  
  if (get_rand() <= 0.90) {
    for (int j=0; j<mvs; ++j) {
      double sc = (double) curr->get_child(j)->score;
      double v = (double) curr->get_child(j)->visits;
      if (v > 0) {
	double r = sc / v;
	double UCT = r + C * sqrt(2.0 * log(N) / v);
	if (UCT > max) { max = UCT; id = j; }
      }
    }
  }

  if (id <0) id = (int)(get_rand() * mvs); // fixme
  return id;
}

int MCTree::pick_child(Board& b) {
  if (!curr->has_children()) curr->add_children(b); 
  return uct_select(); // can be < 0
}

int MCTree::pick_capture(Board& b) {  
  if (!curr->has_children()) curr->add_children(b); // all legal  
  int mvs = curr->num_children();
  
  if (mvs <= 0) return -1;
  
  bool in_check = b.in_check();
  std::vector<int> captures; // store capture indices of curr->children
  
  for (int j=0; j<mvs; ++j) {
    U16 mv = curr->child_move(j);
    bool is_cap = is_capture(mv);    
    if (!in_check && is_cap) captures.push_back(j);
    else if (in_check) captures.push_back(j);
  }

  if (captures.size() <= 0) return -1; // nothing found  
  return captures[(int)(get_rand() * captures.size())];
}

void MCTree::traceback(float score) {
  while (curr->has_parent()) {
    curr = curr->get_parent();
    score = 1 - score;
    curr->visits++; curr->score += score;
  }
}

bool MCTree::search(Board& b) {
  int trials = 0;
  
  while (trials < 60000) {
    Board B(b); curr = root; // needed?
    float score = rollout(B);
    traceback(score);
    ++trials;
  }
  print_pv();
  
  return true;
}

float MCTree::rollout(Board& b) {
  float eval = NINF;
  bool stop = false;
  int moves_searched = 0;
  BoardData bd;
  bool do_resolve = true;

  while(!stop) {

    int id = pick_child(b);
    
    if (id < 0) {
      if (b.in_check()) eval = 0; // mate
      else eval = 0.5; // stalemate
      do_resolve = false;
      break;
    }

    auto n = curr->get_child(id);
    U16 move = curr->child_move(id);
    curr = n;
    
    b.do_move(bd, move);
    
    if (++moves_searched >= 6 &&
	!b.in_check() &&
	!b.gives_check(move)) stop = true;
  }

  if (do_resolve) eval = resolve(b);
  curr->visits++;
  curr->score = eval;
  
  return eval;
}

float MCTree::resolve(Board& b) {
  float eval = NINF;
  bool stop = false;
  int cutoff = 90;
  BoardData bd;
  
  while (!stop) {

    int id = pick_capture(b);
    if (id < 0) { // no captures/evasions
      if (b.in_check()) return 1; // mate
      else { stop = true; break; }
    } 

    auto n = curr->get_child(id);
    U16 move = curr->child_move(id);
    curr = n;
    
    b.do_move(bd, move);
  }

  eval = Eval::evaluate(b);
  eval = (eval <= -cutoff ? 0 : eval <= cutoff ? 0.5 : 1);

  return (1-eval);
}

void MCTree::print_pv() {
  std::vector<U16> PV;
  float max = NINF;
  auto node = root;
  int id = -1;

  while (node->has_children()) {
    max = NINF; id = -1;
    
    for (unsigned int j=0; j<node->num_children(); ++j) {
      auto child = node->get_child(j);      
      if (child->visits > 0) {
	float p = (float) ((float)child->score / (float) child->visits);
	if (p > max) { max = p; id = j; }

	U16 move = node->child_move(j);
	printf("idx(%d), move(%s), visits(%3.3f), wins(%3.3f), p(%3.3f)\n", j,
               UCI::move_to_string(move).c_str(),
               child->visits, 
               child->score,
               p);        
      }
    }
    if (id >= 0) {
      PV.push_back(node->child_move(id));
      node = node->get_child(id);
    }
    else break;
  }
  std::string pv = "";
  if (PV.size() <= 0) { printf("pv (none)\n"); return; }
  for (unsigned int j = 0; j < PV.size(); ++j) {
    pv += UCI::move_to_string(PV[j]) + " ";      
  }
  
  std::cout << std::endl;
  std::cout << "info pv " << pv << std::endl;
  std::cout << "bestmove " << UCI::move_to_string(PV[0]) << std::endl;
}
