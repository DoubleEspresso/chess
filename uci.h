#ifndef UCI_H
#define UCI_H

#include <iostream>
#include <cstring>
#include <stdio.h>
#include <algorithm>
#include <string>

#include "position.h"
#include "bits.h"
#include "types.h"
#include "move.h"

std::string START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

namespace uci {
  void loop();
  bool parse_command(const std::string& input);
}


void uci::loop() {
  std::string input = "";
  while (std::getline(std::cin, input)) {
    if (!parse_command(input)) break;
  }
}

bool uci::parse_command(const std::string& input) {
  std::istringstream instream(input);
  std::string cmd;
  bool running = true;
  
  while (instream >> std::skipws >> cmd) {
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
  
    if (cmd == "position" && instream >> cmd) {
      std::istringstream fen(START_FEN);
      position p(cmd == "startpos" ? fen : instream);      
      p.print();

      movegen<quiet, pawn, white> pawn_mvs(p);      
      pawn_mvs.print();

      movegen<capture, pawn, white> pawn_caps(p);
      pawn_caps.print();
      
    }
    
    else if (cmd == "exit" || cmd == "quit") {
      running = false;
      break;
    }
    else std::cout << "unknown command: " << cmd << std::endl;
    
  }
  return running;
}

#endif
