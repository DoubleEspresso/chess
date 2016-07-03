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
	  //std::cout << "\n " << count << " of " << size << " are legal\n";
	  std::cout << (b.whos_move() == WHITE ? "\nwhite " : "\nblack ") << "to move" << std::endl;
	  std::cout << "castle state white : (" << (b.can_castle(W_KS) ? "k.side" : "") << (b.can_castle(W_QS) ? " q.side" : "") << ")"
		    << " black : (" << (b.can_castle(B_KS) ? "k.side" : "") << (b.can_castle(B_QS) ? " q.side" : "") << ")" << std::endl;
	  //std::cout << "en passant square : " << SanSquares[b.get_ep_sq()] << std::endl;
	  std::cout << "Zobrist key: " << b.pos_key() << std::endl;
	  std::cout << "Evaluation: " << Eval::evaluate(b) << std::endl;

	}
      else if (command == "table")
	{
	  //int w_sc = square_score<WHITE, PAWN>(MIDDLE_GAME, E2);
	  //int b_sc = square_score<BLACK, PAWN>(MIDDLE_GAME, E7);
	  //printf("..dbg sqs score = w_%d, b_%d\n", w_sc, b_sc);
	}
      else if (command == "bits")
	{
	  //U64 tmp = b.all_pieces();
	  //Array::print(tmp);
	}
      else if (command == "legal_mvs")
	{


	}
      else if (command == "caps" || command == "captures")
	{
	  //for (MoveGenerator mvs(b, CAPTURE); !mvs.end(); ++mvs)
	  //	std::cout << SanSquares[get_from(mvs.move())] << SanSquares[get_to(mvs.move())] << " ";
	  //std::cout << "" << std::endl;
	}
      /*
      else if (command == "qsearch")
	{
	  for (MoveGenerator mvs(b, false); !mvs.end(); ++mvs)
	    {
	      U16 m = mvs.move();
	      std::cout << move_to_string(m);
	    }
	  std::cout << std::endl;
	}
      */
      else if (command == "pseudo_mvs")
	{
	  int count = 0;
	  int size = 0;
	  for (MoveGenerator mvs(b, PSEUDO_LEGAL); !mvs.end(); ++mvs)
	    {						
	      //std::cout << SanSquares[get_from(mvs.move())] << SanSquares[get_to(mvs.move())] << " ";
	      U16 m = mvs.move();
	      std::string smv = move_to_string(m);
	      std::cout << smv << " ";
	      if (b.is_legal(m)) count++;
	      size++;
	    }
	  //std::cout << "" << std::endl;
	  //std::cout << " " << count << " of " << size << " are legal" << std::endl;		    
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
	  ROOT_DEPTH = (limits.depth == 0 ? 64 : limits.depth);
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
  else if (type == PROMOTION_CAP)
    {
      Piece pp = Piece(type - 4);
      fromto += SanPiece[pp + 6];
    }
  return fromto;
}

/*
  std::string UCI::move_to_san(U16& m)
  {
  int from = get_from(m);
  int to_sq = get_to(m);
  std::string result;

  char cp = SanPiece[(int)b.piece_on(from)];

  // parse the type of move
  int type = int((m & 0xf000) >> 12);
  bool is_capture = (type == CAPTURE);
  bool is_promotion = (type <= PROMOTION);
  bool is_promotion_cap = (type > PROMOTION && type <= PROMOTION_CAP);
  bool is_castle = (type == CASTLE_KS || type == CASTLE_QS);
  //bool is_ep = (type == EP); // not necessary
  bool gives_check = b.does_check(m);

  // the piece moving
  std::string piece(1, cp);
  piece = (piece == "P") ? "" : piece;
  bool is_pawn = (piece == "");

  // disambiguation
  int ipiece = (int)b.piece_on(from);
  U64 pieces = b.get_pieces(b.whos_move(), ipiece);
  U64 attackers = b.attackers_of(to_sq) & pieces;
  if (more_than_one(attackers))
  {
  int row = ROW(from);
  int col = COL(from);
  int tmp_sq = 0;

  // step 1. check if the files differ
  U64 col_check = COLS[col] & attackers;
  if (!more_than_one(col_check))
  {
  while (attackers)
  {
  tmp_sq = pop_lsb(attackers);
  if (tmp_sq == from)
  {
  std::string tmp_from = SanSquares[tmp_sq];
  piece += tmp_from[0]; // only the file
  }
  else continue;
  }
  }
  // step 2. check if the ranks differ
  else
  {
  while (attackers)
  {
  tmp_sq = pop_lsb(attackers);
  if (tmp_sq == from)
  {
  // get the rank
  std::string tmp_from = SanSquares[tmp_sq];
  piece += tmp_from[1]; // only the rank
  }
  else continue;
  }
  }
  } // end disambiguation


  // castle moves
  if (is_castle)
  {
  result = (type == CASTLE_KS ? "O-O" : "O-O-O");
  return result;
  }
  // captures
  else if (is_capture)
  {
  if (!is_pawn) piece += "x";
  else
  {
  // pawns only!
  std::string to = SanSquares[get_to(m)];
  std::string from = SanSquares[get_from(m)];

  // drop the "row"	  
  result = piece + from[0] + "x" + to;
  if (gives_check) result += "+";
  return result;
  }
  }
  // promotions
  else if (is_promotion)
  {
  std::string to = SanSquares[get_to(m)];
  char promoted_piece = SanPiece[(int)(type)];
  std::string pp(1, promoted_piece);
  to += "=" + pp;
  result = piece + to;
  if (gives_check) result += "+";
  return result;
  }
  else if (is_promotion_cap)
  {
  std::string to = SanSquares[get_to(m)];
  char promoted_piece = SanPiece[(int)(type - 4)];
  std::string pp(1, promoted_piece);
  to += "=" + pp;
  result = piece + to;
  if (gives_check) result += "+";
  return result;
  }
  std::string to = SanSquares[get_to(m)];
  result = piece + to;
  // checks -- mate ??
  if (gives_check) result += "+";
  return result;
  }


  void UCI::PrintBest(Board b)
  {
  int j = 0;
  U16 bestmove[2];
  Entry e;
  for (j = 0; j<2; ++j)
  {
  U64 pk = b.pos_key();

  if (ttable.fetch(pk, e))
  {
  Data * pd = new Data();
  if (j<2) bestmove[j] = e.move;
  if (bestmove[j]) b.do_move(*pd, bestmove[j]);
  }
  else break;

  }
  while (j >= 0) b.undo_move(bestmove[j--]);
  //if (UCI_SIGNALS.stop)
  //{
  U16 mv0 = (bestmove[0] == MOVE_NONE ? RootMoves[0] : bestmove[0]);
  U16 mv1 = (bestmove[1] == MOVE_NONE ? RootMoves[1] : bestmove[1]);
  std::cout << "bestmove " << SanSquares[get_from(mv0)] << SanSquares[get_to(mv0)] << " ponder "
  << SanSquares[get_from(mv1)] << SanSquares[get_to(mv1)] << std::endl;
  //}
  }

  void UCI::run_testing(int fixed_time, int fixed_depth)
  {
  for (int j = 0; j< TestSuite.size(); ++j)
  {
  printf(" ...loading EPD test positions type - %s\n", TestSuite[j].desc.c_str());
  for (int i = 0; i<TestSuite[j].tests.size(); ++i)
  {
  TestPosition test;
  test = TestSuite[j].tests[i];

  // load the position
  int gameover;
  std::string pos = "fen " + test.fen;
  uci_command(pos, gameover);
  std::string go_cmd = "go";

  // setup the limits
  if (fixed_depth > 0) go_cmd += " depth " + String::itoa(fixed_depth);
  else go_cmd += " depth 10"; // fixed depth

  if (fixed_time > 0) go_cmd += " movetime " + String::itoa(fixed_time);
  else go_cmd += " movetime 5000"; // 5 secs.

  // search position
  uci_command(go_cmd, gameover);
  Threads.wait_for_search_finished();

  // done searching, post to UI
  printf(" *** Test(%s), bestmove %s \n", TestSuite[j].desc.c_str(), test.bestmove.c_str());
  printf("\n");

  // record best move stats
  std::string bestmove = move_to_san(RootMoves[0]);
  if (!test.scores.empty())
  {
  std::vector<std::string> scs = String::split(test.scores, ",");
  for (int i = 0; i<scs.size(); ++i)
  {
  std::vector<std::string> tmp = String::split(scs[i], "=");
  if (bestmove == tmp[0])
  {
  TestSuite[j].total += atoi(tmp[1].c_str());
  TestSuite[j].mean += atoi(tmp[1].c_str());
  }
  }
  }
  else if (bestmove == test.bestmove) { TestSuite[j].total += 10; TestSuite[j].mean += 10; }

  }
  }

  // final printout
  for (int j = 0; j< TestSuite.size(); ++j)
  {
  TestSuite[j].mean /= float(TestSuite[j].tests.size());
  printf(" ..final stats for test set %s..\n", TestSuite[j].desc.c_str());
  printf(" total = %3.3f / %d\n", TestSuite[j].total, 10 * TestSuite[j].tests.size());
  printf(" mean  = %3.3f\n", TestSuite[j].mean);
  }

  }
*/
