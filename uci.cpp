#include "uci.h"
#include "move.h"
#include "bench.h"

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
  position p;
  
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
