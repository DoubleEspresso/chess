/*
-----------------------------------------------------------------------------
This source file is part of the Havoc chess engine
Copyright (c) 2020 Minniesoft
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
#include "uci.h"
#include "move.h"
#include "bench.h"
#include "search.h"
#include "threads.h"
#include "hashtable.h"

position p;
Move dbgmove;
Threadpool worker(1);
signals UCI_SIGNALS;

void uci::loop() {
  p.params = eval::Parameters;

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
      
      std::string tmp;
      if (cmd == "startpos") {
        getline(instream, tmp);
        std::istringstream fen(START_FEN);
        p.setup(fen);
        load_position(tmp);
      }
      else {
        std::string sfen = "";
        while ((instream >> cmd) && cmd != "moves") sfen += cmd + " ";
        getline(instream, tmp);
        tmp = "moves " + tmp;
        std::istringstream fen(sfen);
        p.setup(fen);
        load_position(tmp);
      }
      
      p.print();
      std::cout << "position hash key: " << p.key() << std::endl;
      std::cout << "position rep key:" << p.repkey() << std::endl;
    }
    else if (cmd == "d") {
      p.print();
      std::cout << "position hash key: " << p.key() << std::endl;
      std::cout << "fen: " << p.to_fen() << std::endl;
    }
    else if (cmd == "eval") {
      p.print();
      std::cout << "position hash key: " << p.key() << std::endl;
      std::cout << "evaluation: " << eval::evaluate(p, -1) << std::endl;
    }
    else if (cmd == "undo") {
      p.undo_move(dbgmove);
    }
    else if (cmd == "fdepth" && instream >> cmd) {
      p.params.fixed_depth = atoi(cmd.c_str());
      std::cout << "fixed depth search: " << p.params.fixed_depth << std::endl;
    }
    
    else if (cmd == "see" && instream >> cmd) {
      Movegen mvs(p);
      mvs.generate<pseudo_legal, pieces>();
      Move move;

      
      for (int i=0; i<mvs.size(); ++i) {

        if (!p.is_legal(mvs[i])) continue;
	
        if (move_to_string(mvs[i]) == cmd) {
          move = mvs[i];
          break;
        }
	
      }
      
      if (move.type != Movetype::no_type) {
        int score = p.see_move(move);
        std::cout << "See score:  " << score << std::endl;
      }
      else std::cout << " (dbg) See : error, illegal move." << std::endl;
    }
    
    else if (cmd == "domove" && instream >> cmd) {
      Movegen mvs(p);
      bool isok = false;
      mvs.generate<pseudo_legal, pieces>();
      for (int i = 0; i < mvs.size(); ++i) {
        if (!p.is_legal(mvs[i])) continue;
        std::string tmp = SanSquares[mvs[i].f] + SanSquares[mvs[i].t];
        std::string ps = "";
        Movetype t = Movetype(mvs[i].type);
        
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
          dbgmove.set(mvs[i].f, mvs[i].t, Movetype(mvs[i].type));
          isok = true;
          break;
        }
      }
      if (isok) {
        std::cout << "doing mv " << std::endl;
        p.do_move(dbgmove);
      }
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
    else if (cmd == "tune") {
      Perft perft;
      perft.auto_tune();
    }
    else if (cmd == "bench" && instream >> cmd) {
      Perft perft;
      int depth = atoi(cmd.c_str());
      perft.bench(depth, true);
    }
    else if (cmd == "debug") {
      p.debug_search = !p.debug_search;
      std::cout << "debugging set to: " << p.debug_search << std::endl;
    }


    // game specific uci commands (refactor?)
    else if (cmd == "isready") {	
      ttable.clear();
      std::cout << "readyok" << std::endl;
    }
    else if (!Search::searching && cmd == "go") {
      limits lims;
      memset(&lims, 0, sizeof(limits));

      while (instream >> cmd)
      {
        if (cmd == "wtime" && instream >> cmd) lims.wtime = atoi(cmd.c_str());
        else if (cmd == "btime" && instream >> cmd) lims.btime = atoi(cmd.c_str());
        else if (cmd == "winc" && instream >> cmd) lims.winc = atoi(cmd.c_str());
        else if (cmd == "binc" && instream >> cmd) lims.binc = atoi(cmd.c_str());
        else if (cmd == "movestogo" && instream >> cmd) lims.movestogo = atoi(cmd.c_str());
        else if (cmd == "nodes" && instream >> cmd) lims.nodes = atoi(cmd.c_str());
        else if (cmd == "movetime" && instream >> cmd) lims.movetime = atoi(cmd.c_str());
        else if (cmd == "mate" && instream >> cmd) lims.mate = atoi(cmd.c_str());
        else if (cmd == "depth" && instream >> cmd) lims.depth = atoi(cmd.c_str());
        else if (cmd == "infinite") lims.infinite = (cmd == "infinite" ? true : false);
        else if (cmd == "ponder") lims.ponder = atoi(cmd.c_str());
      }
      bool silent = false;
      worker.enqueue(Search::start, p, lims, silent);
    }
    else if (cmd == "stop") {
      UCI_SIGNALS.stop = true;
    }
    else if (cmd == "moves") {
      Movegen mvs(p);
      mvs.generate<pseudo_legal, pieces>();
      for (int i = 0; i < mvs.size(); ++i) {
        //if (!p.is_legal(mvs[i])) continue;
        std::cout << (SanSquares[mvs[i].f] + SanSquares[mvs[i].t]) << " ";
      }
      std::cout << std::endl;
    }
    else if (cmd == "ucinewgame") {      
      ttable.clear();      
    }
    else if (cmd == "uci") {
      ttable.clear();
      std::cout << "id name haVoc" << std::endl;
      std::cout << "uciok" << std::endl;
    }
    
    else if (cmd == "exit" || cmd == "quit") {
      running = false;
      break;
    }
    else std::cout << "unknown command: " << cmd << std::endl;
    
  }
  return running;
}


void uci::load_position(const std::string& pos) {
  std::string token;
  std::istringstream ss(pos);

  ss >> token; // eat the moves token
  while (ss >> token) {
    Movegen mvs(p);
    mvs.generate<pseudo_legal, pieces>();
    for (int j = 0; j < mvs.size(); ++j) {
      if (!p.is_legal(mvs[j])) continue;
      
      if (move_to_string(mvs[j]) == token) {
        p.do_move(mvs[j]);
        break;
      }

    }
  }
  
}

std::string uci::move_to_string(const Move& m) {
  std::string fromto = SanSquares[m.f] + SanSquares[m.t];
  Movetype t = Movetype(m.type);

  std::string ps = "";

  ps = (t == capture_promotion_q ? "q" :
    t == capture_promotion_r ? "r" :
    t == capture_promotion_b ? "b" :
    t == capture_promotion_n ? "n" :
    t == promotion_q ? "q" :
    t == promotion_r ? "r" :
    t == promotion_b ? "b" :
    t == promotion_n ? "n" : "");
  
  return fromto + ps;
}

