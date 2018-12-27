#include "uci.h"
#include "move.h"
#include "bench.h"

position p;
Move dbgmove;

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

      
      Movegen mvs(p);
      mvs.generate<pseudo_legal_all, pieces>();
      
      int count = 0;
      for (int i=0; i<mvs.size(); ++i) {
	if (p.is_legal(mvs[i])) {
	  //p.do_move(m);
	  //p.undo_move(m);
	  ++count;
	}
      }
      p.print();
      mvs.print();
      std::cout << "made " << count << " mvs of " << mvs.size() << std::endl;
      
      
    }
    else if (cmd == "d") {
      p.print();
    }
    else if (cmd == "undo") {
      p.undo_move(dbgmove);
    }
    else if (cmd == "domove" && instream >> cmd) {
      Movegen mvs(p);
      bool isok = false;
      mvs.generate<pseudo_legal_all, pieces>();
      for (int i=0; i<mvs.size(); ++i) {
	if (!p.is_legal(mvs[i])) continue;
	std::string tmp = SanSquares[mvs[i].from()] + SanSquares[mvs[i].to()];
	std::string ps = "";	
	Movetype t = mvs[i].type();
	
	if (t >= 0 && t < capture_promotion_q) {
	  ps = (t == 0 ? "q" :
		t == 1 ? "r" :
		t == 2 ? "b" : "n");	  
	}
	else if (t >= capture_promotion_q && t < castle_ks) {
	  ps = (t == 4 ? "q" :
		t == 5 ? "r" :
		t == 6 ? "b" : "n");
	}
	tmp += ps;
	
	if (tmp == cmd) {
	  dbgmove.set(mvs[i].piece(), mvs[i].from(), mvs[i].to(), mvs[i].type());
	  isok = true;
	  break;
	}	
      }
      if (isok) p.do_move(dbgmove);
      else std::cout << cmd << " is not a legal move" << std::endl;
    }
    else if (cmd == "perft" && instream >> cmd) {
      Perft perft;
      perft.go(atoi(cmd.c_str()));
    }
    else if (cmd == "gen" && instream >> cmd) {
      Perft perft;
      U64 xs = U64(atoi(cmd.c_str()));
      perft.gen(p, xs);
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
