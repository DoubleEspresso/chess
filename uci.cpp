#include <cmath>
#include <stdio.h>
#include <iostream>
#include <cstring>

#include "uci.h"
#include "bench.h"
#include "board.h"
#include "move.h"
#include "search.h"
#include "threads.h"
#include "evaluate.h"
#include "hashtable.h"
#include "pawns.h"
#include "material.h"
#include "utils.h"
#include "globals.h"
#include "opts.h"

using namespace UCI;

Board b;

//std::vector<EPDTest> TestSuite;

void UCI::cmd_loop()
{
  int GAME_FINISHED = 0;
  std::string user_input;
  while (!GAME_FINISHED)
    {
      while (std::getline(std::cin, user_input))
	{
	  uci_command(user_input, GAME_FINISHED);
	  if (GAME_FINISHED) break;
	}
    }
}


void UCI::uci_command(std::string cmd, int& GAME_OVER)
{
  std::istringstream uci_instream(cmd);
  std::string command;
  while (uci_instream >> std::skipws >> command)
    {
      //-------------- start dbg commands -----------------//  
      if (command == "fen")
	{
	  //b.from_fen(uci_instream);
	}
      else if (command == "d" || command == "print")
	{
	  b.print();
	  int count = 0;
	  int size = 0;
	  for (MoveGenerator mvs(b); !mvs.end(); ++mvs)
	    {
	      U16 m = mvs.move();
	      std::cout << move_to_string(m) << " ";
	      if (b.is_legal(m)) count++;
	      size++;
	    }	
	  std::cout << (b.whos_move() == WHITE ? "\nwhite " : "\nblack ") << "to move" << std::endl;
	  std::cout << "castle state white : (" << (b.can_castle(W_KS) ? "k.side" : "") << (b.can_castle(W_QS) ? " q.side" : "") << ")"
		    << " black : (" << (b.can_castle(B_KS) ? "k.side" : "") << (b.can_castle(B_QS) ? " q.side" : "") << ")" << std::endl;
	  std::cout << "Zobrist key: " << b.pos_key() << std::endl;
	  std::cout << "Evaluation: " << Eval::evaluate(b) << std::endl;
	  std::cout << "Fen: " << b.to_fen() << std::endl;
	}


      else if (command == "caps" || command == "captures")
	{
	  for (MoveGenerator mvs(b, CAPTURE); !mvs.end(); ++mvs)
	  {
		U16 m = mvs.move();
		std::cout << move_to_string(m) << " ";
	  }
	  std::cout << "" << std::endl;
	}
      else if (command == "checks")
	{
		MoveGenerator mvs;
		mvs.generate_qsearch_caps_checks(b);
	  for (; !mvs.end(); ++mvs)
	  {
		U16 m = mvs.move();
		std::cout << move_to_string(m) << " ";
	  }
	  std::cout << "" << std::endl;
	}
      else if (command == "pseudo_mvs")
	{
	  int count = 0;
	  int size = 0;
	  printf(".....quiet pseudo legal moves......\n");
	  MoveGenerator mvs; mvs.generate_pseudo_legal(b, QUIET);
	  for (; !mvs.end(); ++mvs)
	    {						
	      U16 m = mvs.move();
	      std::string smv = move_to_string(m);	      
	      std::cout << smv << " ";
	      if (b.is_legal(m)) count++;
	      size++;
	    }
	  printf("\n.....capture pseudo legal moves......\n");
	  MoveGenerator mvs2; mvs2.generate_pseudo_legal(b, CAPTURE);
	  for (; !mvs2.end(); ++mvs2)
	    {
	      U16 m = mvs2.move();
	      std::string smv = move_to_string(m);	      
	      std::cout << smv << " ";
	      if (b.is_legal(m)) count++;
	      size++;
	    }
	  printf("\n...%d moves total, %d are legal\n", size, count);
	}
      else if (command == "bench_pawns")
	{
	  //Benchmark bench(PAWN_POS, 1000000);
	}
      else if (command == "bench_minors")
	{
	  //Benchmark bench(MINORS, 1000000);
	}
      else if (command == "bench" && uci_instream >> command)
	{
	  Benchmark bench(BENCH, atoi(command.c_str()));
	}
      else if (command == "divide" && uci_instream >> command)
	{
	  Benchmark bench;
	  bench.divide(b, atoi(command.c_str()));
	}
      else if (command == "material")
	{
	  //MaterialEntry * me = NULL;
	  //material.get(b);
	  //if (me != NULL)
	  //{
	  //	printf("..(dbg) material = %d\n", me->value);
	  //}
	}
      else if (command == "pawns")
	{
	  //printf("<===== MIDDLE GAME PAWNS =====>\n");
	  //pawnTable.get(b, MIDDLE_GAME);

	  //printf("<===== END GAME PAWNS =====>\n");
	  //pawnTable.get(b, END_GAME);
	}
      else if (command == "eval")
	{
	  int score = Eval::evaluate(b);
	  printf("Score = %d\n", score);
	}
      else if (command == "search" && uci_instream >> command)
	{
	  //if (!b.has_position()) { printf("..no position loaded\n"); return; }
	  //ROOT_BOARD = b;
	  //ROOT_DEPTH = atoi(uci_str.c_str());
	  //Threads.start_thinking(ROOT_BOARD);
	}
      else if (command == "see" && uci_instream >> command)
	{
	  MoveGenerator mvs(b);
	  U16 move = MOVE_NONE;
	  for (; !mvs.end(); ++mvs)
	    {
	      U16 m = mvs.move();
	      if (move_to_string(m) == command)
		{
		  move = m;
		  break;
		}
	    }
	  if (move != MOVE_NONE)
	    {
	      int val = b.see_move(move);
	      std::cout << " (dbg) See score:  " << val << std::endl;
	    }
	  else std::cout << " (dbg) See : error, illegal move." << std::endl;
		


	  //int square = SQUARE_NONE;
	  //for (int j = 0; SanSquares[j] != command && j < SQUARES; ++j) square = j + 1;

	  //if (square != SQUARE_NONE && b.piece_on(square) != PIECE_NONE)
	  //{
	  //	int val = b.see(square);
	  //	std::cout << " (dbg) See score:  " << val << std::endl;
	  //}
	  //else std::cout << " (dbg) See : error, illegal or unoccupied square." << std::endl;
	}
      //-------------- end dbg commands -----------------//

      //-------------- UCI commands ---------------------//
      else if (command == "uci")
	{
	  std::cout << "id name " << ENGINE_NAME << std::endl;
	  std::cout << "uciok" << std::endl;
	}
      else if (command == "isready")
	{
	  std::cout << "readyok" << std::endl;
	}
      else if (command == "candidates")
	{
	  b.print();
	  printf("----discovered candidates for side to move---\n");
	  b.compute_discovered_candidates(b.whos_move());
	  U64 dc = b.discovered_checkers(b.whos_move());
	  U64 db = b.discovered_blockers(b.whos_move());
	  display(dc); display(db);
	  printf("----discovered candidates for side *NOT* to move---\n");
	  b.compute_discovered_candidates(b.whos_move()^1);
	  dc = b.discovered_checkers(b.whos_move()^1);
	  db = b.discovered_blockers(b.whos_move()^1);
	  display(dc); display(db);
	}
      else if (command == "ucinewgame")
	{
	  //ttable.clear();
	  //b.clear();
	}
      else if (command == "help")
	{
	  // command menu here.
	}
      else if (command == "go")
	{
	  // in case we are pondering
	  if (options["pondering"])
	    {
	      UCI_SIGNALS.stop = true;
	      timer_thread->searching = false;
	    }

	  if (!b.has_position()) { printf("..no position loaded\n"); return; }
	  Limits limits;
	  memset(&limits, 0, sizeof(limits));

	  while (uci_instream >> command)
	    {
	      if (command == "wtime" && uci_instream >> command) limits.wtime = atoi(command.c_str());
	      else if (command == "btime" && uci_instream >> command) limits.btime = atoi(command.c_str());
	      else if (command == "winc" && uci_instream >> command) limits.winc = atoi(command.c_str());
	      else if (command == "binc" && uci_instream >> command) limits.binc = atoi(command.c_str());
	      else if (command == "movestogo" && uci_instream >> command) limits.movestogo = atoi(command.c_str());
	      else if (command == "nodes" && uci_instream >> command) limits.nodes = atoi(command.c_str());
	      else if (command == "movetime" && uci_instream >> command) limits.movetime = atoi(command.c_str());
	      else if (command == "mate" && uci_instream >> command) limits.mate = atoi(command.c_str());
	      else if (command == "depth" && uci_instream >> command) limits.depth = atoi(command.c_str());
	      else if (command == "infinite") limits.infinite = (command == "infinite" ? true : false);
	      else if (command == "ponder") limits.ponder = atoi(command.c_str());
	    }
	  
	  hashTable.clear();
	  material.clear();
	  pawnTable.clear();	  
	  RootMoves.clear();
	  timer_thread->search_limits = &limits;

	  ROOT_BOARD = b;
	  ROOT_DEPTH = (limits.depth == 0 ? MAXDEPTH : limits.depth);
	  ROOT_DEPTH = (ROOT_DEPTH > MAXDEPTH ? MAXDEPTH : ROOT_DEPTH <= 0 ? MAXDEPTH : ROOT_DEPTH);
	  UCI_SIGNALS.stop = false;
	  Threads.start_thinking(ROOT_BOARD);
	}
      else if (command == "position" && uci_instream >> command)
	{
	  std::string tmp;

	  if (command == "startpos")
	    {
	      getline(uci_instream, tmp);
	      std::istringstream fen(START_FEN);
	      b.from_fen(fen);
	      load_position(tmp);
	    }
	  else
	    {
	      std::string sfen = "";
	      while ((uci_instream >> command) && command != "moves") sfen += command + " ";
	      getline(uci_instream, tmp);
	      tmp = "moves " + tmp;

	      std::istringstream fen(sfen);
	      b.from_fen(fen);
	      load_position(tmp);
	    }
	}
      else if (command == "stop" || command == "exit" || command == "quit" )
	{
	  UCI_SIGNALS.stop = true;
	  timer_thread->searching = false;
	  if (command == "exit" || command == "quit" ) GAME_OVER = 1;
	}
      else std::cout << "..unknown command " << command << std::endl;
    }
}

void UCI::analyze(Board& b)
{
  Limits limits;
  memset(&limits, 0, sizeof(limits));
  limits.infinite = true;

  hashTable.clear();
  material.clear();
  pawnTable.clear();
  RootMoves.clear();
  
  timer_thread->search_limits = &limits;
  
  //std::memcpy(&ROOT_BOARD, &b, sizeof(Board));
  ROOT_BOARD = b;
  ROOT_DEPTH = 64;
  UCI_SIGNALS.stop = false;
  //ttable.clear(); 
  Threads.start_thinking(ROOT_BOARD);  
  Threads.wait_for_search_finished();
}

void UCI::load_position(std::string& pos)
{
  std::string token;
  std::istringstream ss(pos);

  ss >> token; // eat the moves token
  while (ss >> token)
    {
      move_from_string(token);
    }
}

U16 UCI::get_move(std::string& move)
{
  MoveGenerator mvs(b);

  for (; !mvs.end(); ++mvs)
    {
      U16 m = mvs.move();
      if (move_to_string(m) == move)
	{
	  return m;
	}
    }
  return MOVE_NONE;
}
void UCI::move_from_string(std::string& move)
{
  MoveGenerator mvs(b);

  for (; !mvs.end(); ++mvs)
    {
      U16 m = mvs.move();
      if (move_to_string(m) == move)
	{
	  BoardData * pd = new BoardData();
	  b.do_move(*pd, m);
	  break;
	}
    }
}

std::string UCI::move_to_string(U16& m)
{
  int from = int(m & 0x3f);
  int to = int((m & 0xfc0) >> 6);
  int type = get_movetype(m); // int((m & 0xf000) >> 12);

  std::string fromto = SanSquares[from] + SanSquares[to];

  if (type != MOVE_NONE && type <= PROMOTION )
    {
      Piece pp = Piece(type);
      fromto += SanPiece[pp + 6];
    }
  else if (type <= PROMOTION_CAP && type > PROMOTION)
    {
      Piece pp = Piece(type - 4);
      fromto += SanPiece[pp + 6];
    }
  return fromto;
}
