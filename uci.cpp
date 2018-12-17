#include "uci.h"
#include "move.h"

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
      if (p.in_check()) {
	std::cout << "..in check - gen evasions" << std::endl;
      }
      mvs.generate<pseudo_legal_all, pieces>();
      mvs.print();
      Move mv1 = mvs[0];
      p.do_move(mv1);
      p.print();
    }
    
    else if (cmd == "exit" || cmd == "quit") {
      running = false;
      break;
    }
    else std::cout << "unknown command: " << cmd << std::endl;
    
  }
  return running;
}
