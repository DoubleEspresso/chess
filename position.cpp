#include <algorithm>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>

#include "position.h"


position::position(std::istringstream& fen) { setup(fen); }
						    
void position::setup(std::istringstream& fen) {
  clear();
  std::string token;
  fen >> token;
  Square s = Square::A8;
  
  for(auto& c : token) {
    if (isdigit(c)) s += int(c - '0');
    else if (c == '/') s -= 16;
    else { set_piece(c, s); ++s; }
  }
  
  // side to move
  fen >> token;
  ifo.stm = (token == "w" ? Color::white : Color::black);

  // the castle rights
  fen >> token;
  ifo.cmask = U16(0);
  for (auto& c : token) ifo.cmask |= CastleRights.at(c);
  
  // ep square
  fen >> token;
  ifo.eps = Square::no_square;
  Row row = Row::no_row; Col col = Col::no_col;
  for (auto& c : token) {
    if (c >= 'a' && c <= 'h') col = Col(c - 'a');
    if (c == '3' || c == '6') row = Row(c - '1');
  }
  ifo.eps = Square(8 * row + col);

  // half-moves since last pawn move/capture
  fen >> ifo.move50;
  
  // move counter
  fen >> ifo.hmvs;
}

void position::do_move(const U16& m) {
  history.emplace_back(ifo);
  
  // update ks if king move, castle rights
  // update castle rights if rook move
  // clear captured piece
  // switch type of move
  // update eps, captured piece, pinned, in check, stm
  // update move 50, half-mvs, check-info
}

void position::set_piece(const char& p, const Square& s) {
  auto idx = std::distance(SanPiece.begin(), std::find(SanPiece.begin(), SanPiece.end(), p));
  if (idx < 0) return; // error
  
  Color color = (idx < 6 ? white : black);
  Piece piece = Piece(idx < 6 ? idx : idx - 6);  
  pcs.set(color, piece, s);
    
  // update zobrist keys
  // update material entries
  // update pawn keys
}

void position::clear() {
  pcs.clear();
  history.clear();
  ifo = {};
  ci.reset();
}

void position::print() {
  std::cout << "   +---+---+---+---+---+---+---+---+" << std::endl;
  for (Row r = r8; r >= r1; --r) {    
    std::cout << " " << r + 1 << " ";
    
    for (Col c = A; c <= H; ++c) {
      Square s = Square(8 * r + c);
      if (pcs.piece_on[s] != no_piece) {
        Piece p = pcs.piece_on[s];
        std::cout << "| "
                  << (pcs.color_on[s] == Color::white ? SanPiece[p] : SanPiece[p+6])
                  << " ";
      }
      else std::cout << "|   ";	
    }
    std::cout << "|" << std::endl;
    std::cout << "   +---+---+---+---+---+---+---+---+" << std::endl;
  }
  std::cout << "     a   b   c   d   e   f   g   h  " << std::endl;
}
