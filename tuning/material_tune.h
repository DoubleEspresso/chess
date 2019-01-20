#ifndef MATERIAL_TUNE_H
#define MATERIAL_TUNE_H

#include <iomanip>

#include "../move.h"
#include "../position.h"
#include "../types.h"
#include "../utils.h"

#include "pgn.h"


struct material_score {
  double num_pawns[2];
  double num_knights[2];
  double num_bishops[2];
  double num_rooks[2];
  double num_queens[2];
  double avg_diff;
  Result result;
  double pawn_val, knight_val, bishop_val, rook_val, queen_val;

  material_score() { clear(); }

  inline void clear() {
    for (int i=0; i<2; ++i) {
      num_pawns[i] = 0; num_knights[i] = 0; num_bishops[i] = 0;
      num_rooks[i] = 0; num_queens[i] = 0; result = Result::pgn_none;
    }
    pawn_val = knight_val = bishop_val = rook_val = queen_val = 1.0;    
  }
  
  inline double score(const Color& c) {
    return pawn_val * num_pawns[c] +
      knight_val * num_knights[c] +
      bishop_val * num_bishops[c] +
      rook_val * num_rooks[c] +
      queen_val * num_queens[c];
  }
  
  inline bool has_pawns() const {
    return num_pawns[white] != 0 || num_pawns[black] != 0;
  }
  
  inline double knights_diff() const {
    return num_knights[white] - num_knights[black];
  }

  inline double bishops_diff() const {
    return num_bishops[white] - num_bishops[black];
  }

  inline double rooks_diff() const {
    return num_rooks[white] - num_rooks[black];
  }

  inline double queens_diff() const {
    return num_queens[white] - num_queens[black];
  }
  
  inline double pawns_diff() const {
    return num_pawns[white] - num_pawns[black];
  }

  inline double total_knights() const {
    return num_knights[white] + num_knights[black];
  }

  inline double total_bishops() const {
    return num_bishops[white] + num_bishops[black];
  }

    inline double total_rooks() const {
    return num_rooks[white] + num_rooks[black];
  }

    inline double total_queens() const {
    return num_queens[white] + num_queens[black];
  }
  
  inline double minors_diff() const {
    return (num_knights[white] + num_bishops[white]) -
      (num_knights[black] + num_bishops[black]);
  }

  inline double pieces_diff() const {
    return knights_diff() + bishops_diff() +
      rooks_diff() + queens_diff();
  }
  
  inline double material_advantage() {
    return score(white) - score(black);
  }


  inline void refresh(const position& p) {
    clear();
    
    for (Color c = white; c <= black; ++c ) {

      U64 pawns = (c == white ? p.get_pieces<white, pawn>() : p.get_pieces<black, pawn>());
      Square * knights = (c == white ? p.squares_of<white, knight>() : p.squares_of<black, knight>());
      Square * bishops = (c == white ? p.squares_of<white, bishop>() : p.squares_of<black, bishop>());
      Square * rooks = ( c == white ? p.squares_of<white, rook>() : p.squares_of<black, rook>());
      Square * queens = (c == white ? p.squares_of<white, queen>() : p.squares_of<black, queen>());
      
        
      while(pawns) { ++num_pawns[c]; pop_lsb(pawns); }
    
      for (Square s = *knights; s != no_square; s = *++knights) { ++num_knights[c]; }
      
      for (Square s = *bishops; s != no_square; s = *++bishops) { ++num_bishops[c]; }
      
      for (Square s = *rooks; s != no_square; s = *++rooks) { ++num_rooks[c]; }
      
      for (Square s = *queens; s != no_square; s = *++queens) { ++num_queens[c]; }
    }
  }  
};


class material_tune {
  std::vector<game> games;
  std::vector<material_score> md;
  
  void analyze();
  
 public:
  material_tune() { md.clear(); }
  
 material_tune(std::vector<game> g) : games(g) { md.clear(); analyze(); }    
  
};


void material_tune::analyze() {

  double pawn_cap_count = 0;
  double pp_count = 0;
  double pn_count = 0;
  double pb_count = 0;
  double pr_count = 0;
  double pq_count = 0;
  
  for (const auto& g : games) {

    position p;
    std::istringstream fen(START_FEN);    
    p.setup(fen); // start position for each game
    size_t count = 0;
    material_score ms;    
    size_t qcount = 0;
    
    for (const auto& m : g.moves) {

      if (m.type == quiet) ++qcount;
      else qcount = 0;
      
      ms.refresh(p);
      if (qcount > 1 && ms.pawns_diff() > 0 && count < 28 &&
	  ms.minors_diff() == -1 && ms.knights_diff() == -1 && ms.rooks_diff() == 0 && ms.queens_diff() == 0 && g.result == Result::pgn_draw) {
	pn_count += ms.pawns_diff();
	++pawn_cap_count;
      }
      /*
      if (m.) {
	ms.refresh(p);
	if (ms.piece_diff()p.piece_on(Square(m.f)) == pawn) {
	  ++pawn_cap_count;
	  if (p.piece_on(Square(m.t)) == pawn) ++pp_count;
	  if (p.piece_on(Square(m.t)) == knight) ++pn_count;
	  if (p.piece_on(Square(m.t)) == bishop) ++pb_count;
	  if (p.piece_on(Square(m.t)) == rook) ++pr_count;
	  if (p.piece_on(Square(m.t)) == queen) ++pq_count;
	}
      }
      */
      
      p.do_move(m);
      
      ++count;	  
    }

    /*
    if (count < 30) continue;

    ms.refresh(p); // update counts    
    ms.result = g.result;
    md.push_back(ms);
    */

  }
  //std::cout << "avg pp freq : " << (pp_count / pawn_cap_count) << std::endl;
  std::cout << "avg p diff, down 1 knight : " << (pn_count / pawn_cap_count) << std::endl;
  //std::cout << "avg pb freq : " << (pb_count / pawn_cap_count) << std::endl;
  //std::cout << "avg pr freq : " << (pr_count / pawn_cap_count) << std::endl;
  //std::cout << "avg pq freq : " << (pq_count / pawn_cap_count) << std::endl;  

  /*
  double win_prob_1pawn = 0;
  double tot_1pawn = 0;
  double win_prob_2pawn = 0;
  double tot_2pawn = 0;
  double win_prob_3pawn = 0;
  double tot_3pawn = 0;
  double win_prob_1knight = 0;
  double tot_1knight = 0;
  double win_prob_1bishop = 0;
  double tot_1bishop = 0;
  double win_prob_exchange = 0;
  double tot_1exchange = 0;
  double win_prob_rook = 0;
  double avg_pawn_count = 0;
  */

  /*
  for (const auto& m : md) {

    if (m.result == Result::pgn_) {
      
      if (m.pieces_diff() != 0 && //knights_diff() == -1 &&
	  //m.total_bishops() == 0 &&
	  //m.rooks_diff() == 0 &&
	  //m.queens_diff() == 0 &&
	  //m.total_rooks() == 0 &&
	  //m.total_queens() == 0 && 
	  m.has_pawns())
	{ avg_pawn_count += m.pawns_diff(); ++tot_1pawn; }
      
      else if (std::abs(m.minors_diff()) == 1 && m.bishops_diff() == 0 &&
	  m.rooks_diff() == 0 &&
	  m.queens_diff() == 0 &&
	  m.pawns_diff() == 1) { ++win_prob_2pawn; }

      
      else if (std::abs(m.minors_diff()) == 1 && m.bishops_diff() == 0 &&
	  m.rooks_diff() == 0 &&
	  m.queens_diff() == 0 &&
	  m.pawns_diff() == 2) { ++win_prob_3pawn; }
      
      else if (std::abs(m.minors_diff()) == 1 && m.bishops_diff() == 0 &&
	  m.rooks_diff() == 0 &&
	  m.queens_diff() == 0 &&
	  m.pawns_diff() == 3) { ++win_prob_1knight; }
      
      
    }

  }
  */
    /*    
    if (m.pieces_diff() == 0) {

      if (m.pawns_diff() == 1) { 
	if (m.result == Result::pgn_wwin) ++win_prob_1pawn;
	++tot_1pawn;
      }
      else if (m.pawns_diff() == -1) {
	if (m.result == Result::pgn_bwin) ++win_prob_1pawn;
	++tot_1pawn;
      }
      
	       
      else if (m.pawns_diff() == 2) {
	if (m.result == Result::pgn_wwin) ++win_prob_2pawn;
	++tot_2pawn;	
      }
      else if (m.pawns_diff() == -2) {
	if (m.result == Result::pgn_bwin) ++win_prob_2pawn;
	++tot_2pawn;
      }
      
      
      else if (m.pawns_diff() == 3) {
	if (m.result == Result::pgn_wwin) ++win_prob_3pawn;
	++tot_3pawn;
      }
      else if (m.pawns_diff() == -3) {
	if (m.result == Result::pgn_bwin) ++win_prob_3pawn;
	++tot_3pawn;
      }
    }
    */
    
    /*
    else if ((m.pawns_diff() == 0 && m.has_pawns() && m.rooks_diff() == 0 && m.queens_diff() == 0) &&
	     (m.minors_diff() == 1 || m.minors_diff() == -1)) {
      
      if (std::abs(m.knights_diff()) == 1 && m.bishops_diff() == 0) {
	if (m.result != Result::pgn_draw) ++win_prob_1knight;
	++tot_1knight;
      }      
      else if (std::abs(m.bishops_diff()) == 1 && m.knights_diff() == 0) {
	if (m.result != Result::pgn_draw) ++win_prob_1bishop;
	++tot_1bishop;
      }

    }
    
    
    
    else if ((m.pawns_diff() == 0 && m.has_pawns() && std::abs(m.rooks_diff()) == 1
	 && m.queens_diff() == 0) && std::abs(m.minors_diff()) == 1) {

      if (m.result != Result::pgn_draw) { ++win_prob_exchange; }
      ++tot_1exchange;
      
    }
  }
    */
  //std::cout << "avg pawn win, down 1 knight : " << (avg_pawn_count / tot_1pawn) << std::endl;  
  //std::cout << "draw down 1 knight, up 1 pawn : " << (win_prob_2pawn / tot_1pawn) << std::endl;
  //std::cout << "draw down 1 knight, up 2 pawns : " << (win_prob_3pawn / tot_1pawn) << std::endl;
  //std::cout << "draw down 1 knight, up 3 pawns : " << (win_prob_1knight / tot_1pawn) << std::endl;
    /*
  std::cout << "1 bishop win prob : " << (win_prob_1bishop / tot_1bishop) << std::endl;
  std::cout << "1 exchange win prob : " << (win_prob_exchange / tot_1exchange) << std::endl;
  */
  
}


#endif
