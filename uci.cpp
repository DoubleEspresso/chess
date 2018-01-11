#include <cmath>
#include <stdio.h>
#include <iostream>
#include <cstring>

#include "uci.h"
#include "globals.h"
#include "magic.h"
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
#include "opts.h"
#include "mctree.h" // monte-carlo searching (dbg)
#include "move2.h"

using namespace UCI;

Board b;

void UCI::loop()
{
  int GAME_FINISHED = 0;
  std::string user_input;
  while (!GAME_FINISHED)
    {
      while (std::getline(std::cin, user_input))
        {
          command(user_input, GAME_FINISHED);
          if (GAME_FINISHED) break;
        }
    }
}

void UCI::command(std::string cmd, int& GAME_OVER)
{
  std::istringstream uci_instream(cmd);
  std::string command;
  while (uci_instream >> std::skipws >> command)
    {
      if (command == "d" || command == "print")
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

      else if (command == "newmoves") {
        MoveGenerator2<QUIETS, PAWN, WHITE> mvs(b);
        MoveGenerator2<CAPTURE_PROMOTIONS, PAWN, WHITE> mvs2(b);
        mvs.print();
        mvs2.print();
      }
      else if (command == "mc" || command == "mcsearch") {
        MCTree mc;
        mc.search(b);
      }
      else if (command == "caps" || command == "captures") {
        for (MoveGenerator mvs(b, CAPTURE); !mvs.end(); ++mvs)
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
      else if (command == "bench" && uci_instream >> command)
        {
          Benchmark bench(BENCH, atoi(command.c_str()));
        }
      else if (command == "xray" && uci_instream >> command)
        {
          U64 wxrays = b.xray_attackers(atoi(command.c_str()), WHITE);
          U64 bxrays = b.xray_attackers(atoi(command.c_str()), BLACK);
          printf("..white-xrays\n");
          PrintBits(wxrays);
          printf("..black-xrays\n");
          PrintBits(bxrays);
        }
      else if (command == "divide" && uci_instream >> command)
        {
          Benchmark bench;
          bench.divide(b, atoi(command.c_str()));
        }
      else if (command == "eval")
        {
          int score = Eval::evaluate(b);
          printf("Score = %d\n", score);
        }
      else if (command == "popcount" && uci_instream >> command)
        {
          Benchmark bench;
          bench.bench_bitcount(atoi(command.c_str()));
        }
      else if (command == "lsb" && uci_instream >> command)
        {
          Benchmark bench;
          bench.bench_lsb(atoi(command.c_str()));
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
              std::cout << "See score:  " << val << std::endl;
              int val2 = b.see_move(move);
              std::cout << "See2 score:  " << val2 << std::endl;
            }
          else std::cout << " (dbg) See : error, illegal move." << std::endl;
        }

      //-------------- UCI commands ---------------------//
      else if (command == "uci")
        {
          UCI_SIGNALS.stop = true;
          timer_thread->searching = false;

          hashTable.clear();
          material.clear();
          pawnTable.clear();
          RootMoves.clear();

          std::cout << "id name " << ENGINE_NAME << std::endl;
          std::cout << "uciok" << std::endl;
        }
      else if (command == "isready")
        {
          UCI_SIGNALS.stop = true;
          timer_thread->searching = false;
	
          hashTable.clear();
          material.clear();
          pawnTable.clear();
          RootMoves.clear();

          std::cout << "readyok" << std::endl;
        }
      else if (command == "candidates")
        {
          b.print();
          printf("----discovered candidates for side to move---\n");
          b.compute_discovered_candidates(b.whos_move());
          U64 dc = b.discovered_checkers(b.whos_move());
          U64 db = b.discovered_blockers(b.whos_move());
          PrintBits(dc); PrintBits(db);
          printf("----discovered candidates for side *NOT* to move---\n");
          b.compute_discovered_candidates(b.whos_move() ^ 1);
          dc = b.discovered_checkers(b.whos_move() ^ 1);
          db = b.discovered_blockers(b.whos_move() ^ 1);
          PrintBits(dc); PrintBits(db);
        }
      else if (command == "ucinewgame")
        {
          UCI_SIGNALS.stop = true;
          timer_thread->searching = false;

          hashTable.clear();
          material.clear();
          pawnTable.clear();
          RootMoves.clear();
        }
      else if (command == "force")
        {
          UCI_SIGNALS.stop = true;
          timer_thread->searching = false;
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
          //pawnTable.clear();
          RootMoves.clear();
          timer_thread->search_limits = &limits;

	  /*
	  MCTree * mc = new MCTree(b);
	  mc->search(4);
	  if (mc) { delete mc; mc = 0; }
	  */

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
      else if (command == "stop" || command == "exit" || command == "quit")
        {
          UCI_SIGNALS.stop = true;
          timer_thread->searching = false;
          if (command == "exit" || command == "quit") GAME_OVER = 1;
        }
      else std::cout << "..unknown command " << command << std::endl;
    }
}

void UCI::analyze(Board& b)
{
  Limits limits;
  memset(&limits, 0, sizeof(limits));
  limits.infinite = true;

  //hashTable.clear();
  material.clear();
  //pawnTable.clear();
  RootMoves.clear();

  timer_thread->search_limits = &limits;
  ROOT_BOARD = b;
  ROOT_DEPTH = 64;
  UCI_SIGNALS.stop = false;
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
	  //if (pd) { delete pd; pd = 0; }
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

  if (type != MOVE_NONE && type <= PROMOTION)
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
