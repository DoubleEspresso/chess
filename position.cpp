#include <algorithm>
#include <string>
#include <vector>
#include <sstream>

#include "position.h"



position::position(std::istringstream& fen) { setup(fen); }
						    
void position::setup(std::istringstream& fen) {
  clear();
  std::string token;
  fen >> token;

  Square s = Square::A8;

  // pieces
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

  // set the king square
  
}

void position::print() {
  
}

void position::set_piece(const char& p, const Square& s) {
  //std::vector<Color> cof{Color::white, Color::white, Color::white};
  for(auto& c : SanPiece) {
    
  }
}


void position::clear() {
  pcs.clear();
  //ifo{};
}
