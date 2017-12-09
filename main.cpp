#include <cstring>
#include <iostream>

#include "opts.h"
#include "info.h"
#include "bits.h"
#include "globals.h"
#include "magic.h"
#include "zobrist.h"
#include "uci.h"
#include "material.h"
#include "hashtable.h"
#include "pawns.h"
#include "threads.h"
#include "pgnio.h"

void parse_args(int argc, char ** argv);
int exit(const char * msg);

int main(int argc, char ** argv) {

  Info::BuildInfo::greeting();
  
  if (!Globals::init()) return exit("..failed to init global arrays, abort!\n");  
  if (!Magic::init()) return exit("..failed to initialize magic mulitpliers for move generation!\n");
  if (!Zobrist::init()) return exit("..failed to initialize zobrist keys, abort!\n");
  if (!material.init()) return exit("..failed to initialize material table, abort!\n");
  if (!pawnTable.init()) return exit("..failed to initialize pawn table, abort!\n");
  if (!hashTable.init()) return exit("failed to initialize hash table, abort!\n");
  
  timer_thread->init();
  Threads.init();
  
  // enter the main UCI command parsing loop
  argc <= 1 ? UCI::loop() : parse_args(argc, argv);
  return EXIT_SUCCESS;
}

int exit(const char * msg) {
  printf("%s\n", msg);
  std::cin.get();
  return EXIT_FAILURE;
}

// collect user input arguments and set settings entries
void parse_args(int argc, char* argv[]) {
  for (int j = 0; j<argc; ++j) {
    // for creating searchable binary files from pgn games data
    if (!strcmp(argv[j], "-pgn")) {
      Board b;
      std::istringstream fen(START_FEN);
      b.from_fen(fen);
      pgn_io pgn(argv[j+1], "testdb.bin", 500);	  
      if (!pgn.parse(b)) {
	printf("..ERROR: failed to parse %s correctly\n", argv[j+1]);
      }
    }
    // debug -- find position in binary file (opening db)
    else if (!strcmp(argv[j], "-find")) {
      //printf("..finding %s\n", argv[j+1]);
      //Board b;
      //std::istringstream fen(argv[j+1]);
      //pgn.find(argv[j+1]);
    }
    else if (!strcmp(argv[j], "-test")) {
      printf("..start testing\n");
      std::istringstream fen ("3q1rk1/r4ppp/1pp5/3pp1n1/1P1n4/P6P/BBPP1PP1/R2Q1RK1 b - - 1 20");
      Board b(fen);
      UCI::analyze(b);

    }
    else if (!strcmp(argv[j], "-match")) printf("..match mode\n");
    else if (!strcmp(argv[j], "-bench")) printf("..benchmark\n");
    else if (!strcmp(argv[j], "-usett")) printf("..set option tt\n");
    else if (!strcmp(argv[j], "-ttsizekb")) printf("..set option ttsizekb\n");
    else if (!strcmp(argv[j], "-nthreads")) printf("..set option nthreads\n");
    else if (!strcmp(argv[j], "-trace")) printf("..set option eval trace\n");
    else if (!strcmp(argv[j], "-stats")) printf("..set option stats\n");
    else if (!strcmp(argv[j], "-log")) printf("..set option log\n");
  }
}

