
#pragma once


#ifndef EPD_H
#define EPD_H


#include <string>
#include <iostream>
#include <fstream>
#include <cassert>
#include <vector>
#include <sstream>


#include "tuning/gen/pgn.h"
#include "position.h"
#include "uci.h"
#include "utils.h"

struct epd_entry {
	std::string pos;
	std::string bestmove;
};

class epd {

	std::vector<epd_entry> positions;
	bool load(const std::string& filename);

public:
	epd() {}
	epd(const std::string& filename) { load(filename); }
	epd(const epd& o) = delete;
	epd(const epd&& o) = delete;
	~epd() {}

	epd& operator=(const epd& o) = delete;
	epd& operator=(const epd&& o) = delete;

	inline std::vector<epd_entry> get_positions() { return positions; }
};



bool epd::load(const std::string& filename) {
	// assuming every line of the epd file is a string..
	std::string line("");
	std::ifstream epd_file(filename);
	size_t counter = 0;
	while (std::getline(epd_file, line)) {

		epd_entry e;
		position p;
		pgn _pgn;

		std::vector<std::string> tokens = util::split(line, ';');

		if (tokens.size() <= 0) {
			std::cout << "skipping invalid line: " << line << std::endl;
		}

		// skip the avoid move positions 

		// the token of interest is the first token
		std::string epd_line = tokens[0];


		if (epd_line.find("bm") == std::string::npos) {
			std::cout << "..skipping invalid line: " << epd_line << std::endl;
			continue;
		}


		e.pos = epd_line.substr(0, epd_line.find(" bm "));
		e.bestmove = epd_line.substr(epd_line.find("bm ") + 3); // to the end

		//std::cout << "dbg: pos: " << e.pos << " bm: " << e.bestmove << std::endl;

		// conver SAN -> from-to move
		std::istringstream pstream(e.pos);
		p.setup(pstream);
		Move m;
		_pgn.move_from_san(p, e.bestmove, m);
		e.bestmove = uci::move_to_string(m);
		positions.push_back(e);

		//std::cout << "  dbg: parsed bm: " << e.bestmove << std::endl;

		++counter;
	}
	std::cout << "loaded " << counter << " test positions" << std::endl;

	return true;
}

#endif