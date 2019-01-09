#ifndef PGN_H
#define PGN_H

#include <fstream>
#include <algorithm>
#include <sstream>
#include <string>
#include <iostream>

#include "../move.h"
#include "../position.h"
#include "../types.h"

enum Result { pgn_draw, pgn_wwin, pgn_bwin, pgn_none };


struct game {
  Result result;
  std::vector<Move> moves;
  
  game() { clear(); }
  bool finished() { return result != Result::pgn_none; }
  void clear() { moves.clear(); result = Result::pgn_none; }
};


class pgn {
 private:
  std::vector<game> games;
  std::vector<std::string> pgn_files;
  
  bool parse_files();
  bool parse_moves(const std::string& line, game& g);

  inline bool is_header(const std::string& line);
  inline bool is_empty(const std::string& line);
  
 public:
  pgn(const std::vector<std::string>& files);
  ~pgn() { }
  pgn(const pgn& o)=delete;
  pgn(const pgn&& o)=delete;
  pgn& operator=(const pgn& o)=delete;
  pgn& operator=(const pgn&& o)=delete;

  
  std::vector<game>& parsed_games() { return games; }
  
};



inline bool pgn::is_header(const std::string& line) {
  return (line.find("[") != std::string::npos ||
	  line.find("]") != std::string::npos);
}


inline bool pgn::is_empty(const std::string& line) {
  return (line.size() <= 0 || line == "\n");
}

#endif
