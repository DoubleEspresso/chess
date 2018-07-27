#ifndef UCI_H
#define UCI_H

#include <iostream>
#include <cstring>
#include <stdio.h>
#include <algorithm>
#include <string>


namespace uci {
  void loop();
  bool parse_command(const std::string& input);
}


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
    
    if (cmd == "exit" || cmd == "quit") {
      running = false;
      break;
    }
    else std::cout << "unknown command: " << cmd << std::endl;
    
  }
  return running;
}

#endif
