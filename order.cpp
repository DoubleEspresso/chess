#include "order.h"
#include "move.h"
#include "position.h"


move_history& move_history::operator=(const move_history& mh) {  
  std::copy(std::begin(mh.history), std::end(mh.history), std::begin(history));
  return (*this);
}

void move_history::update(const position& p,
			  const Move& m,
			  const Move& previous,
			  const int16& depth,
			  const std::vector<Move>& quiets) {

  const Color c = p.to_move();
  int score = pow(depth, 2);
  
  if (m.type == Movetype::quiet) {
    history[c][m.f][m.t] += score;
  }

  if (previous.type == Movetype::quiet) {
    history[c ^ 1][previous.f][previous.t] -= score;
  }  
  
  for (auto& q : quiets) {
    history[c][q.f][q.t] -= score;
  }
  
  // killers
}



void move_history::clear() { 
  for (auto& v: history) { for (auto& w : v) { std::fill(w.begin(), w.end(), 0); } }  
}


template<>
int move_history::score<white>(const Move& m) {
  return history[white][m.f][m.t];
}


template<>
int move_history::score<black>(const Move& m) {
  return history[black][m.f][m.t];
}

