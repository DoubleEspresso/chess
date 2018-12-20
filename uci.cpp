#include "uci.h"
#include "move.h"
#include "bench.h"

position p;

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
      p.setup(cmd == "startpos" ? fen : instream);
      p.print();

      /*
      Movegen mvs(p);
      mvs.generate<pseudo_legal_all, pieces>();
      
      int count = 0;
      for (int i=0; i<mvs.size(); ++i) {
	Move m = mvs.move(i);
	if (p.is_legal(m)) {
	  p.do_move(m);
	  p.undo_move(m);
	  ++count;
	}
      }
      p.print();
      mvs.print();
      std::cout << "made " << count << " mvs of " << mvs.size() << std::endl;
      */
      
    }
    else if (cmd == "domove" && instream >> cmd) {
      Movegen mvs(p);
      Move m;
      bool isok = false;
      mvs.generate<pseudo_legal_all, pieces>();
      for (int i=0; i<mvs.size(); ++i) {
	if (!p.is_legal(mvs.move(i))) continue;
	std::string tmp = SanSquares[mvs[i].from()] + SanSquares[mvs[i].to()];
	if (tmp == cmd) {
	  m = mvs[i];
	  isok = true;
	  break;
	}	
      }
      if (isok) p.do_move(m);
      else std::cout << cmd << " is not a legal move" << std::endl;
    }
    else if (cmd == "perft" && instream >> cmd) {
      Perft perft;
      perft.go(atoi(cmd.c_str()));
    }
    else if (cmd == "divide" && instream >> cmd) {
      Perft perft;
      perft.divide(p, atoi(cmd.c_str()));
    }
    else if (cmd == "exit" || cmd == "quit") {
      running = false;
      break;
    }
    else std::cout << "unknown command: " << cmd << std::endl;
    
  }
  return running;
}
