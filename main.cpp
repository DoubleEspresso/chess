#include <cstring>
#include <iostream>
#include <map>
#include <memory>

#include "options.h"
#include "info.h"
#include "types.h"
#include "utils.h"
#include "bitboards.h"
#include "bits.h"
#include "magics.h"
#include "uci.h"

std::unique_ptr<options> opts;

int main(int argc, char * argv[]) {

  opts = std::unique_ptr<options>(new options(argc, argv));
  
  {
    // setup
    info::greeting();
    bitboards::load();  
    magics::load();
  }

  uci::loop();
  
  return 0;
}
