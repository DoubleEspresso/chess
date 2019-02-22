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
			  const Score& eval,
			  const std::vector<Move>& quiets,
			  Move * killers) {
  
  const Color c = p.to_move();
  int score = pow(depth, 2);
  
  if (m.type == Movetype::quiet) {
    history[c][m.f][m.t] += score;

    if (eval >= Score::mate_max_ply &&
	m != killers[0] &&
	m != killers[1] &&
	m != killers[2]) {
      killers[3] = killers[2];
      killers[2] = m;
    }    
    else if ( eval < Score::mate_max_ply &&
	      m != killers[2] &&
	      m != killers[3] &&
	      m != killers[0]) {
      killers[1] = killers[0];
      killers[0] = m;
    }
    
  }

  if (previous.type == Movetype::quiet) {
    history[c ^ 1][previous.f][previous.t] -= score;
  }  
  
  for (auto& q : quiets) {
    if (m.f == q.f) continue;
    history[c][q.f][q.t] -= score;
  }
  
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


scored_move& scored_move::operator=(const scored_move& o) { m = o.m; s = o.s; return (*this); }



move_order::move_order(position& p,
		       Move& hashmv,
		       Move * kill) :
  phase(hash_move), hashmove(&hashmv),
  killers(kill), to_move(p.to_move()), incheck(p.in_check()) {
  
  if (!p.is_legal_hashmove(hashmv)) { hashmove->f = Square(0); hashmove->t = Square(0); hashmove->type = Movetype::no_type; }
  
  stats = &(p.history_stats());
  moves = new Movegen(p);
  list.clear();
}

move_order::~move_order() {
  if (moves) { delete moves; moves = 0; }
}


bool move_order::skip(const Move& m) {
  if (m.type == Movetype::no_type) return true;
  
  if (m == *hashmove) return true;

  
  if ((m == killers[0] || m == killers[1] ||
       m == killers[2] || m == killers[3]))
    return true;
  

  return false;
}

void move_order::sort() {
  unsigned N = list.size();  
  scored_move key({}, Score(0));
  int j;

  for (unsigned i = 1; i < N; ++i) {
    key = list[i]; 
    j = i-1;  
    while (j >= 0 && list[j] > key) {
      list[j+1] = list[j];
      --j;
    }
    list[j+1] = key; 
  }

}



template<>
bool move_order::next_move<main>(position& pos, Move& m) {

  m = {};
  m.type = Movetype::no_type;
  std::vector<int> mvals { 10, 30, 35, 48, 91, 200 };
  
  switch (phase) {
    
  case hash_move: {
    if (hashmove->type != Movetype::no_type) {
      m = *hashmove;
    }
    ++phase;
    break;
  }

    
  case mate_killer1 : {
    if (pos.is_legal_hashmove(killers[2]) &&
	killers[2] != *hashmove) {
      m = killers[2];
    }
    ++phase; break;
  }

    
  case mate_killer2 : {
    if (pos.is_legal_hashmove(killers[3])) {
      if (killers[3] != *hashmove && killers[3] != killers[2])
	m = killers[3];
    }
    ++phase; break;
  }

    
  case good_captures : {
    if (list.size() <= 0) {
      
      moves->generate<capture, pieces>();
      for (int i = 0; i < moves->size(); ++i) {

	if (skip((*moves)[i])) continue;
	
	Piece pt = pos.piece_on(Square((*moves)[i].t));
	Piece pf = pos.piece_on(Square((*moves)[i].f));	
	
	Score sc = ((*moves)[i].type == Movetype::ep) ? Score(0) :
	  Score(mvals[pt] - mvals[pf]);
	
	list.emplace_back(scored_move((*moves)[i], sc));
      }
      
      sort();
      
      if (list.size() <= 0) { ++phase; }
    }
    else {
      if (list.back().s >= Score(0)) {
	m = list.back().m;
	list.pop_back();
      }

      if (list.size() > 0 && list.back().s < Score(0)) { ++phase; }
      if (list.size() <= 0) { moves->reset(); ++phase; }      
    }    
    break;
  }


  case killer1 : {
    if (pos.is_legal_hashmove(killers[0])) {
      if (killers[0].f != hashmove->f || killers[0].t != hashmove->t)
	m = killers[0];
    }
    ++phase;
    break;
  }
    

  case killer2 : {
    if (pos.is_legal_hashmove(killers[1])) {
      if ((killers[1].f != hashmove->f || killers[1].t != hashmove->t) &&
	  (killers[1].f != killers[0].f || killers[1].t != killers[0].t))
	m = killers[1];
    }
    ++phase;
    break;
  }
    
    
  case bad_captures: {
    if (list.size() <= 0) { moves->reset(); ++phase; return true; } 
    else {
      m = list.back().m;
      list.pop_back();
    }
    break;
  }
    

  case quiets : {
    if (list.size() <= 0) {
      
      auto ss = [this](const Move& m) {
	return Score(to_move == white ?
		     stats->score<white>(m) : stats->score<black>(m)); };
      
      moves->generate<quiet, pieces>();
      for (int i = 0; i < moves->size(); ++i) {

	if (skip((*moves)[i])) { continue; }
	
	Score sc = Score(ss((*moves)[i]));
	
	list.emplace_back(scored_move((*moves)[i], sc));

      }
      sort();
    }

    if (list.size() <= 0) { moves->reset(); ++phase; }
    else {
      m = list.back().m;
      list.pop_back();
      if (list.size() <= 0) { moves->reset(); ++phase; return true; } 
    }
    break;
  }
    
  case end : { return false; }
    
  } // end switch
  
  return phase != OrderPhase::end;
}




template<>
bool move_order::next_move<qsearch>(position& pos, Move& m) {
  
  m = {};
  m.type = Movetype::no_type;
  std::vector<int> mvals { 10, 30, 35, 48, 91, 200 };
  
  switch (phase) {
    
  case hash_move: {
    if (incheck ||
	hashmove->type == Movetype::capture ||
	hashmove->type == Movetype::ep ||
	hashmove->type == Movetype::capture_promotion_q ||
	hashmove->type == Movetype::capture_promotion_r ||
	hashmove->type == Movetype::capture_promotion_b ||
	hashmove->type == Movetype::capture_promotion_n) {      
      m = *hashmove;
    }
    ++phase;
    break;
  }

  case mate_killer1 : {
    if (pos.is_legal_hashmove(killers[2])) {
      if (killers[2].f != hashmove->f || killers[2].t != hashmove->t)
	m = killers[2];
    }
    ++phase;
    break;
  }
    

  case mate_killer2 : {
    if (pos.is_legal_hashmove(killers[3])) {
      if ((killers[3].f != hashmove->f || killers[3].t != hashmove->t) &&
	  (killers[3].f != killers[2].f || killers[3].t != killers[2].t))	
	m = killers[3];
    }
    ++phase;
    break;
  }
    
  case good_captures : {
    if (list.size() <= 0) {
      
      moves->generate<capture, pieces>();
      for (int i = 0; i < moves->size(); ++i) {
	
	if (skip((*moves)[i])) continue;
	
	Piece pt = pos.piece_on(Square((*moves)[i].t));
	Piece pf = pos.piece_on(Square((*moves)[i].f));
	
	Score sc = ((*moves)[i].type == Movetype::ep) ? Score(0) :
	  Score(mvals[pt] - mvals[pf]);
	
	
	list.emplace_back(scored_move((*moves)[i], sc));
      }      
      
      sort();
      
      
      if (list.size() <= 0) { ++phase; }
    }
    else {
      if (list.back().s >= Score(0)) {
	m = list.back().m;
	list.pop_back();
      }

      if (list.size() > 0 && list.back().s < Score(0)) { ++phase; }
      if (list.size() <= 0) { moves->reset(); ++phase; }      
    }
    
    break;
  }

    
  case killer1 : {
    if (incheck && pos.is_legal_hashmove(killers[0])) {
      if (killers[0].f != hashmove->f || killers[0].t != hashmove->t)
	m = killers[0];
    }
    ++phase;
    break;    
  }
    
    
  case killer2 : {
    if (incheck && pos.is_legal_hashmove(killers[1])) {
      if ((killers[1].f != hashmove->f || killers[1].t != hashmove->t) &&
	  (killers[1].f != killers[0].f || killers[1].t != killers[0].t))
	m = killers[1];
    }
    ++phase;
    break;
  }   

    
  case bad_captures: {
    if (list.size() <= 0) { moves->reset(); ++phase; return true; } 
    else {
      m = list.back().m;
      list.pop_back();
    }
    break;
  }

    
  case quiets : {
    if (list.size() <= 0 && incheck) {
      
      auto ss = [this](const Move& m) {
		  return Score(to_move == white ?
			       stats->score<white>(m) : stats->score<black>(m)); };
      
      moves->generate<quiet, pieces>();
      for (int i = 0; i < moves->size(); ++i) {

	if (skip((*moves)[i])) continue;
	
	Score sc = Score(ss((*moves)[i]));
	
	list.emplace_back(scored_move((*moves)[i], sc));

      }
      sort();
    }

    if (list.size() <= 0) { moves->reset(); ++phase; }
    else {
      m = list.back().m;
      list.pop_back();
      if (list.size() <= 0) { moves->reset(); ++phase; return true; } 
    }
    break;
  }

  case end : { return false; }
    
  } // end switch
  
  return phase != OrderPhase::end;
}
