#ifndef HEDWIG_PGNIO_H
#define HEDWIG_PGNIO_H

#include <string.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

#include "board.h"
#include "move.h"
#include "book.h"

// a stripped move is encoded as a from|to squares, each sq needs only 6 bits
enum Tag
{
  EVENT, SITE, PGNDATE, ROUND, TWHITE, TBLACK, RESULT, WELO, BELO, TAG_NB
};

struct pgn_data 
{
  U64 key;  
  std::vector<U16> moves;   
};

// attributed packed for win/unix see e.g. 
// http://stackoverflow.com/questions/1537964/visual-c-equivalent-of-gccs-attribute-packed
PACK( struct db_entry
{
  U64 key;
  U16 move;
});

class pgn_io
{
  std::fstream * ofile;
  std::ifstream * ifile;
  Book * book;
  pgn_data * data;
  size_t nb_elements;
  size_t size_bytes;
  std::string tags[TAG_NB];
  int move_count;
  int collision_count;
  
  public:
  pgn_io(char * db_fname);
  pgn_io(char * pgn_fname, char * db_fname, size_t size_mb);
  ~pgn_io();
  
  bool load(Board& b);
  bool parse(Board& b);   
  bool write();
  bool init_db(size_t size_mb);
  std::string find(const char * fen);
  
  bool parse_tags(std::string& line);
  bool parse_moves(Board& b, BoardData& pd, std::string& line, bool& eog);
  bool parse_header_tag(std::string& line);
  
  // given a san move - return the U16 encoded move
  U16 san_to_move_16(Board& b, std::string& s);
  U16 encode_move(U16& m);
  
  int parse_piece(char& c);
  int to_square(std::string& s);
  U16 find_move_row(Board& b, int row, int to, int piece);
  U16 find_move_col(Board& b, int col, int to, int piece);
  U16 find_move_promotion(Board& b, int pp, int to, int fp, bool isCapture);
  U16 find_move(Board& b, int to, int piece);
  U16 get_opening_move(const char * fen);
  
  // given a U16 encoded move, convert to proper SAN format.
  char * move_to_san(U16 & m);
  void pgn_strip(std::string& move);
  
  // header tag hash function (FNV)
  unsigned int FNV_hash(const char* key, int len);
};

#endif