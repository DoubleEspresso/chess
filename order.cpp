#include "order.h"
#include "move.h"


move_history& move_history::operator=(const move_history& mh) {  
  std::copy(std::begin(mh.history), std::end(mh.history), std::begin(history));
  return (*this);
}

void move_history::update(const Move& m,
			  const Move& previous,
			  const int16& depth,
			  Move * quiets) {
  std::cout << "update!" << std::endl;
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

