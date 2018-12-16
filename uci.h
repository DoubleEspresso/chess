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
      
      
      Movegen mvs(p);
      mvs.generate<quiet, pawn>();
      mvs.generate<capture, pawn>();
      mvs.generate<quiet, knight>();
      mvs.print();

      
      /*
      movegen<capture, pawn, white> pawn_caps(p);
      pawn_caps.print();

      movegen<quiet, pawn, black> bpawn_mvs(p);      
      bpawn_mvs.print();

      movegen<capture, pawn, black> bpawn_caps(p);
      bpawn_caps.print();

      movegen<quiet, knight, black> bknight_mvs(p);
      bknight_mvs.print();

      movegen<quiet, knight, white> wknight_mvs(p);
      wknight_mvs.print();

      movegen<quiet, bishop, white> wbish_mvs(p);
      wbish_mvs.print();

      movegen<quiet, rook, white> wrook_mvs(p);
      wrook_mvs.print();

      movegen<quiet, queen, white> wq_mvs(p);
      wq_mvs.print();

      movegen<quiet, king, white> wk_mvs(p);
      wk_mvs.print();
      */
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
