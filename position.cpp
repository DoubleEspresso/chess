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
    else switch (c) {
      case '/':
        s -= 16;
        break;
      default:
        set_piece(c, s); ++s;
        break;
      }
  }
  
  // side to move
  fen >> token;
  ifo.stm = (token == "w" ? Color::white : Color::black);

  // castle rights
  fen >> token;
  ifo.cmask = U16(0);
  for (auto& c : token) {
    auto i = std::distance(CastleFen.begin(), std::find(CastleFen.begin(), CastleFen.end(), c));
    ifo.cmask |= U16(1 << i);
  }
  
  // ep square
  fen >> token;
  ifo.eps = Square::no_square;
  if (token.size() == 2) {
    if (token[0] >= 'a' && 
        token[0] <= 'h' && 
        token[1] == '3' &&
        token[1] == '6') 
      ifo.eps = Square(8 * (int(token[0]-'a')) + (int(token[1] - '1')));
  }

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

// utilities
void position::clear() {
  pcs.clear();
  ifo = {};
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
