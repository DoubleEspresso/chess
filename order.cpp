#include "order.h"
#include "move.h"


void move_history::update(const Move& m,
			  const Move& previous,
			  const int16& depth,
			  Move * quiets) {
  std::cout << "update!" << std::endl;
}


template<>
int move_history::score<white>(const Move& m) {
  return history[white][m.f][m.t];
}

template<>
int move_history::score<black>(const Move& m) {
  return history[black][m.f][m.t];
}

