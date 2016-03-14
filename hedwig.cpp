
#include "info.h"
#include "bits.h"
#include "globals.h"
#include "magic.h"
#include "zobrist.h"
#include "uci.h"
#include "material.h"
#include "hashtable.h"
#include "threads.h"

void parse_args(int argc, char ** argv);

int main(int argc, char ** argv)
{
	// simple info/greeting to user
	Info::BuildInfo::greeting();

	// fill the global chess data arrays for moves/analysis
	if (Globals::init()) printf("..global arrays initialized\n");

	// the magic bitboard initialization
	Magic::init();
	if (!Magic::check_magics())
	{
		printf("..!!ERROR : attack hash failed to initialize properly, abort!\n");
		std::cin.get();
		return EXIT_FAILURE;
	}
	else printf("..attack hash arrays initialized\n");

	// fill the zobrist arrays for transposition table hashing
	if (!Zobrist::init())
	{
		printf("..!!ERROR : failed to initialize zobrist keys, abort!\n");
		std::cin.get();
		return EXIT_FAILURE;
	}
	else printf("..zobrist keys initialized\n");


	// initialize the various hash tables used during the search/evaluation
	if (!material.init())
	{
		printf("..!!ERROR : failed to initialize material table, abort!\n");
		std::cin.get();
		return EXIT_FAILURE;
	}

	if (!hashTable.init())
	{
		printf("..!!ERROR : failed to initialize hash table, abort!\n");
		std::cin.get();
		return EXIT_FAILURE;
	}

	// initialize the threadpool (timer thread, master + worker threads)
	timer_thread->init();
	Threads.init();

	// enter the main UCI command parsing loop
	argc <= 1 ? UCI::cmd_loop() : parse_args(argc, argv);

	return EXIT_SUCCESS;
}


// collect user input arguments and set settings entries
// TODO: include a usage method here.
void parse_args(int argc, char* argv[])
{
	bool dotest = false;
	int testtime = 0;
	int testdepth = 0;
	for (int j = 0; j<argc; ++j)
	{
		if (!strcmp(argv[j], "-test"))
		{
			//std::ifstream file(argv[j + 1]);
			//std::string line, curr_id, past_id;
			//EPDTest et;
			//past_id = "";
			//int count = 0;
			//while (std::getline(file, line))
			//{
			//	TestPosition tp;
			//	curr_id = String::parse_epd(line, tp);
			//	if (past_id != curr_id && et.tests.size() > 0)
			//	{
			//		TestSuite.push_back(et);
			//		et.tests.clear();
			//		printf(" ..loaded %d EPD tests of type %s\n", count, past_id.c_str());
			//		past_id = curr_id;
			//		count = 0;
			//	}
			//	else
			//	{
			//		if (count == 0) past_id = curr_id; // hack for starting
			//		et.desc = curr_id;
			//		et.mean = et.total = 0;
			//		et.tests.push_back(tp);
			//		count++;
			//	}
			//}
			//// the last tests
			//TestSuite.push_back(et);
			//et.tests.clear();
			//printf(" ..loaded %d EPD tests of type %s\n", count, past_id.c_str());

			//dotest = true;
			//j++;
			//if (j >= argc) break;
		}
		else if (!strcmp(argv[j], "-match")) printf("..match mode\n");
		else if (!strcmp(argv[j], "-bench")) printf("..benchmark\n");
		else if (!strcmp(argv[j], "-usett")) printf("..set option tt\n");
		else if (!strcmp(argv[j], "-ttsizekb")) printf("..set option ttsizekb\n");
		else if (!strcmp(argv[j], "-nthreads")) printf("..set option nthreads\n");
		else if (!strcmp(argv[j], "-trace")) printf("..set option eval trace\n");
		else if (!strcmp(argv[j], "-stats")) printf("..set option stats\n");
		else if (!strcmp(argv[j], "-log")) printf("..set option log\n");
		else if (!strcmp(argv[j], "-testtime")) { testtime = atoi(argv[j + 1]); j++; if (j >= argc) break; }
		else if (!strcmp(argv[j], "-testdepth")) { testdepth = atoi(argv[j + 1]); j++; if (j >= argc) break; }
	}

	//if (dotest) UCI::run_testing(testtime, testdepth);
}

