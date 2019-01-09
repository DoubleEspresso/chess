#include "pgn.h"



pgn::pgn(const std::vector<std::string>& files) {
  pgn_files = std::vector<std::string>(files);
  parse_files();
}


bool pgn::parse_files() {

  size_t fcount = 0;
  for(const auto& f : pgn_files) {
    
    std::cout << "..parsing games from " << f << std::endl; 

    std::ifstream pgn_file(f.c_str(), std::fstream::in);

    
    if (!pgn_file.is_open()) {
      std::cout << "..failed to open pgn file, skipping" << std::endl;
      continue;
    }

    std::string line;
    game g;
    
    while (std::getline(pgn_file, line)) {

      if (is_empty(line) || is_header(line)) { continue; }
      
      if (!parse_moves(line, g)) {
	g.clear();
	std::cout << "..error parsing moves from " << f
		  << " skipping." << std::endl;
	break;
      }      
      
      if (g.finished()) {
	games.push_back(g);
	g.clear();
      }
    }
    
    ++fcount;
    pgn_file.close();
  }
  std::cout << "..parsed " << games.size() << " chess games." << std::endl;
  return true;
}


bool pgn::parse_moves(const std::string& line, game& g) {
  std::stringstream ss(line);
  std::string token;

  while(ss >> std::skipws >> token) {
    
    if (token == "1/2-1/2" || token == "1-0" ||
	token == "0-1" || token == "*") {
      
      g.result = (token == "1/2-1/2" ? Result::pgn_draw :
		  token == "1-0" ? Result::pgn_wwin :
		  Result::pgn_bwin);
      
      return true;
    }
  }

  return true;
}
