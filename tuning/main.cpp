#include <vector>
#include <random>
#include <functional>
#include <iostream>
#include <dirent.h>

#include "pbil.h"
#include "pgn.h"
#include "square_tune.h"

#include "../bitboards.h"
#include "../magics.h"
#include "../zobrist.h"

void parse_args(int argc, char * argv[]);


int integer(const std::vector<int>& sample, const int& sidx, const int& eidx) {
  int result = 0;
  for (int j = sidx, s = eidx - 1 - sidx; j < eidx; ++j, --s) {
    result |= (sample[j] << s); // bit ordering is e.g. 4 = {1,0,0}
  }
  return result;
}

// compute error for toy problem : find binary representation of a given "answer" integer.
double residual(std::vector<int> sample, int length, int answer) {
  return std::abs(integer(sample, 0, length) - answer);
}

void problem1() {
  // find binary representation of a given input integer  
  int answer = 93019431;
  int length = (int)log2(answer)+1;
  pbil p(300, length, 0.7, 0.15, 0.3, 0.05, 1e-6); 
  p.optimize(residual, length, answer);    
}


int main(int argc, char * argv[]) {
  
  zobrist::load();
  
  bitboards::load();
  
  magics::load();

  
  parse_args(argc, argv);

  return EXIT_SUCCESS;
}


void parse_args(int argc, char * argv[]) {

  for (int j = 0; j < argc; ++j) {
    
    if (!strcmp(argv[j], "-pgn_dir")) {

      DIR * dir;
      struct dirent * ent;
      std::vector<std::string> pgn_files;
      if ( (dir = opendir(argv[j+1])) != NULL) {

	std::string sdir(argv[j+1]);
	while((ent = readdir(dir)) != NULL) {
	  if (!strcmp(ent->d_name, ".") ||
	      !strcmp(ent->d_name, "..")) {
	      continue;
	    }
	  std::string fname = sdir + "/" + ent->d_name;
	  pgn_files.push_back(fname);
	}
	
	closedir(dir);
	pgn io(pgn_files); // run analysis
	square_tune st(io.parsed_games());
      }
      else {
	std::cout << "..pgn directory "
		  << argv[j+1]
		  << " not found, ignored." << std::endl;
      }
      
    } // end pgn dir check
    
  } // endfor - options
  
  
}
