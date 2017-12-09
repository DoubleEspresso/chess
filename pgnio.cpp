
#include <algorithm>
#include <math.h>

#include "pgnio.h"

// initial position

struct
{
  bool operator()(const pgn_data& x, const pgn_data& y) { return x.key > y.key; }
} PGNGreaterThan;


bool MoveSort(float i, float j) { return (i >= j); }

pgn_io::pgn_io(const char * db_fname) :
  ofile(0), ifile(0), book(0), data(0),
  nb_elements(0), size_bytes(0), move_count(0), collision_count(0) {
  if (db_fname) {
    ofile = new std::fstream(db_fname, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
    if (ofile->is_open())
      printf("...opened database file %s\n", db_fname);
  }
  book = new Book();
}

pgn_io::pgn_io(const char * pgn_fname, const char * db_fname, size_t size_mb) :
  ofile(0), ifile(0), book(0), data(0),
  nb_elements(0), size_bytes(0), move_count(0), collision_count(0)
{
  if (pgn_fname)
    {
      ifile = new std::ifstream();
      ifile->open(pgn_fname, std::ifstream::in);
    }
  if (db_fname)
    {
      ofile = new std::fstream(db_fname, std::ios::in | std::ios::out | std::ios::binary | std::fstream::trunc);
    }
  init_db(size_mb);
  book = new Book();
}

pgn_io::~pgn_io()
{
  //if (ofile) { delete ofile; ofile = 0; }
  //if (ifile) { ifile->close(); delete ifile; ifile = 0; }
  //if (data)  { delete data; data = 0; }
  //if (book)  { delete book; book = 0; }
}

bool pgn_io::init_db(size_t size_mb)
{
  size_t kb = 1024;
  size_t mb = 1024 * kb;
  //size_t size_mb = 10 * size_mb; // size in bytes
  nb_elements = nearest_power_of_2(size_mb*mb) / sizeof(pgn_data);
  size_bytes = nb_elements * sizeof(pgn_data);
  //printf("..dbg size_bytes = %d\n", size_bytes);
  if (!data && (data = new pgn_data[nb_elements]()))
    {
      printf("..initialized db file: size %3.1f mb, %zu elts\n", float(size_bytes) / float(mb), nb_elements);
    }
  return true;
}

bool pgn_io::parse(Board& b)
{
  if (!ifile) return false;
  if (!ifile->is_open()) {
    printf("..ERROR: failed to open pgn file, check filename.\n");
    return false;
  }
  
  for (int j = 0; j < TAG_NB; ++j) tags[j] = "";

  std::string line; BoardData pd;
  bool eog = false; int games = 0;
  while (std::getline(*ifile, line)) {
    // parse line
    if (line.find("[") != std::string::npos || line.find("]") != std::string::npos) {
      parse_header_tag(line);
    }
    else if (line.size() > 0 && line != "\n") {
      //printf("..found line: %s\n", line.c_str());
      if (!parse_moves(b, pd, line, eog)) return false;
      if (eog) {
	// do we have enough space to write a new entry
	if (sizeof(data) >= size_bytes - 1024) {
	  printf("..end pgn parse, space full\n");
	  printf("..%d games, %d moves, %d collisions parsed successfully.", games, move_count, collision_count);
	  write();
	  return true;
	}

	eog = false;
	b.clear();
	std::istringstream fen(START_FEN);
	b.from_fen(fen); ++games;
	for (int j = 0; j < TAG_NB; ++j) tags[j] = "";

      }
    }
  }
  //for (int j=0; j<9; ++j)
  //printf("header tag --> %s\n", data->tags[j].c_str());
  write();
  printf("..finished, %d games, %d moves, %d collisions parsed successfully.", games, move_count, collision_count);
  return true;
}

bool pgn_io::parse_header_tag(std::string& line)
{
  std::stringstream ss(line);
  std::string token;
  while (ss >> std::skipws >> token)
    {
      std::string tag = "";
      std::string key = "";
      pgn_strip(token);
      for (unsigned int j = 0; j < token.size(); ++j) token[j] = tolower(token[j]);

      tag += token;
      while (ss >> std::skipws >> token) {
	pgn_strip(token);
	for (unsigned int j = 0; j < token.size(); ++j) token[j] = tolower(token[j]);
	key += " " + token;
      }
      if (tag == "event") tags[EVENT] = key;
      else if (tag == "site") tags[SITE] = key;
      else if (tag == "date")
	{
	  tags[PGNDATE] = key;
	  //std::cout << FNV_hash(key.c_str(), key.size()) << std::endl;
	}
      else if (tag == "round")
	{
	  tags[ROUND] = key;
	  //std::cout << FNV_hash(key.c_str(), key.size()) << std::endl;
	}
      else if (tag == "white")
	{
	  tags[TWHITE] = key;
	  //std::cout << FNV_hash(key.c_str(), key.size()) << std::endl;
	}
      else if (tag == "black")
	{
	  tags[TBLACK] = key;
	  //std::cout << FNV_hash(key.c_str(), key.size()) << std::endl;
	}
      else if (tag == "whiteelo")
	{
	  tags[WELO] = key;
	  //std::cout << FNV_hash(key.c_str(), key.size()) << std::endl;
	}
      else if (tag == "blackelo")
	{
	  tags[BELO] = key;
	  //std::cout << FNV_hash(key.c_str(), key.size()) << std::endl;
	}
      else if (tag == "result")
	{
	  tags[RESULT] = key;
	  //std::cout << FNV_hash(key.c_str(), key.size()) << std::endl;
	}
    }
  return true;
}

unsigned int pgn_io::FNV_hash(const char * key, int len)
{
  unsigned int h = 2166136261;
  int i;

  for (i = 0; i < len; i++)
    h = (h * 16777619) ^ key[i];

  return h;
}

bool pgn_io::parse_moves(Board& b, BoardData& pd, std::string& line, bool& eog)
{
  std::stringstream ss(line);
  std::string token;
  bool comment = false; std::string comments = "";
  std::vector<std::string> comment_list;
  while (ss >> std::skipws >> token)
    {
      if (token.find("{") != std::string::npos)
	{
	  comment = true;
	}
      else if (token.find("}") != std::string::npos)
	{
	  comment = false;
	  comments += token;
	  comment_list.push_back(comments);
	  comments = "";
	  continue;
	}
      if (comment)
	{
	  comments += token;
	  continue;
	}

      // check move nb formatting (some files record moves as 1. e4, others as 1.e4)
      int idx = -1;
      if ((idx = token.find(".")) != (int)std::string::npos) {
	std::string res = "";
	while (token[idx]) {
	  if (token[idx] != '.') res += token[idx];
	  ++idx;
	}
	token = res;
	if (res == "") continue;
      }
      
      // check end of game, todo: handling various spacings 
      if (token == "1/2-1/2" || token == "1-0" || token == "0-1" || token == "*") {
	//printf("..end of game, %s",(token == "1/2-1/2" ? "draw" : token == "1-0" ? "white win" : "black win"));
	//b.print();	  
	eog = true;
	return true;
      }
      else {
	// strip move of all notations, checks/mates
	pgn_strip(token);
	//if (move_count < 50) printf("token = %s\n", token.c_str());
	U16 m = san_to_move_16(b, token);
	if (m != 0) {
	  ++move_count;
	  book->compute_key(b.to_fen().c_str());
	  U64 k = book->key();
	  b.do_move(pd, m);

	  int idx = (nb_elements - 1) & k;

	  // handle collisions
	  if (data[idx].key != 0 && data[idx].key != k) {
	    ++collision_count;
	    //printf("..collision @ %d, for move = %s\n", idx, token.c_str());
	    for (unsigned int j = idx; j < nb_elements; ++j) {
	      if (data[j].key == 0 || data[j].key == k) {
		--collision_count;
		data[j].key = k;
		data[j].moves.push_back(encode_move(m));
		break;
	      }
	    }
	  }
	  else {
	    data[idx].key = k;
	    data[idx].moves.push_back(encode_move(m));
	  }
	}
	else {
	  printf("...ERROR parsing %s \n", token.c_str());
	  b.print();
	  return false;
	}
      }
    } // finished parsing all moves, insert indices/data into binary file (?)
  return true;
}

U16 pgn_io::encode_move(U16& m)  // insert the game result into the move
{
  U16 em;
  int type = int((m & 0xf000) >> 12);
  int pp = 0;
  // normal promotion moves
  if (type != MOVE_NONE && type <= PROMOTION)
    {
      pp = type;
    }
  // promotion captures
  if (type > PROMOTION && type <= PROMOTION_CAP)
    {
      pp = Piece(type - 4);
    }
  int t = get_to(m);
  int f = get_from(m);
  int r = (tags[RESULT] == " 1-0" ? 1 : tags[RESULT] == " 0-1" ? 2 : 0); // 2 bits
  em = (f | (t << 6) | r << 12 | pp << 14);
  return em;
}

// note: returns 0 on error!
U16 pgn_io::san_to_move_16(Board& b, std::string& s)
{
  // moving backward through SAN string..
  int len = s.size();
  int i = s.size() - 1;
  int to = -1;
  bool isPromotion = false; int promotionType = 0;

  // check castle move
  if (s == "O-O" || s == "O-O-O")
    {
      if (s == "O-O") to = (b.whos_move() == WHITE ? G1 : G8);
      else to = (b.whos_move() == WHITE ? C1 : C8);
      return find_move(b, to, int(KING));
    }
  // check promotion moves
  else if (s[i] == 'Q' || s[i] == 'R' || s[i] == 'B' || s[i] == 'N')
    {
      promotionType = (s[i] == 'N' ? 1 : s[i] == 'B' ? 2 : s[i] == 'R' ? 3 : 4);

      i -= 1;
      if (s[i] == '=')
	{
	  i -= 1;
	  s = s.substr(0, len - 2); // remove the "=q" piece
	  len -= 2;
	}
      else
	{
	  s = s.substr(0, len - 1); // remove the "q" piece, no = 
	  len -= 1;
	}
      to = to_square(s);
      isPromotion = true;
    }
  else to = to_square(s); // todo: catch shorthands like ed for exd4 etc.

  // normal pawn moves and normal promotion moves (non-capture) (a4 etc. which are len 2)
  if (len <= 2 && to >= 0)
    {
      //printf("..dbg in find move %s to = %d\n",s.c_str(), to);
      return (isPromotion ? find_move_promotion(b, promotionType, to, int(PAWN), false) : find_move(b, to, int(PAWN)));
    }

  // either piece move like ra4 or pawn capture like axb4 or promotion capture like axb8=q
  i -= 2;
  char c = s[i];

  // is it x ? 
  if (tolower(c) == 'x') { i -= 1; c = s[i]; } // skip x's

  // if i >= 1, move includes additional info about the from
  // square (either row or col), e.g. raxb3 or r4xb3
  int row = -1; int col = -1;
  if (i >= 1)
    {
      //printf(" .. multiple from sqs for %s\n ", s.c_str());
      if (isdigit(s[i])) row = int(s[i] - '1');
      else col = int(s[i] - 'a');
      i -= 1;
    }

  // piece
  int piece = -1;

  //printf("move = %s, i = %d\n", s.c_str(), i);
  if (i >= 0)
    {
      piece = parse_piece(s[i]);
      //printf("..piece = %d for move %s, promotion = %d\n", piece, s.c_str(), isPromotion);
    }
  if (piece < 0)
    {
      printf("..ERROR : invalid piece parsed from PGN move!\n");
      return 0;
    }
  else if (piece == 0)
    {
      //printf("..pawn capture %s\n", s.c_str());
      col = int(s[0] - 'a'); // in case of pawn captures, we need to know the col (from sq)
    }

  // return move
  //printf("row=%d, col=%d\n", row, col);
  if (row != -1) return (isPromotion ? find_move_promotion(b, promotionType, to, int(PAWN), true) : find_move_row(b, row, to, piece));
  else if (col != -1) return (isPromotion ? find_move_promotion(b, promotionType, to, int(PAWN), true) : find_move_col(b, col, to, piece));
  else return (isPromotion ? find_move_promotion(b, promotionType, to, int(PAWN), true) : find_move(b, to, piece));

  return 0;
}

U16 pgn_io::find_move(Board& b, int to, int piece)
{

  std::vector<U16> candidates;
  for (MoveGenerator mvs(b); !mvs.end(); ++mvs)
    {
      U16 m = mvs.move();
      int p = b.piece_on(get_from(m));
      if (get_to(mvs.move()) == to && (piece == p)) candidates.push_back(m);
    }

  return (candidates.size() == 1 ? candidates[0] : 0);
}

U16 pgn_io::find_move_row(Board& b, int row, int to, int piece)
{
  std::vector<U16> candidates;
  for (MoveGenerator mvs(b); !mvs.end(); ++mvs)
    {
      U16 m = mvs.move();
      int f = get_from(m);
      int p = b.piece_on(f);
      if (get_to(mvs.move()) == to && (piece == p) && (ROW(f) == row)) candidates.push_back(m);
    }
  return (candidates.size() == 1 ? candidates[0] : 0);
}

U16 pgn_io::find_move_col(Board& b, int col, int to, int piece)
{
  std::vector<U16> candidates;
  for (MoveGenerator mvs(b); !mvs.end(); ++mvs)
    {
      U16 m = mvs.move();
      int f = get_from(m);
      int p = b.piece_on(f);
      if (get_to(mvs.move()) == to && (piece == p) && (COL(f) == col)) candidates.push_back(m);
    }
  return (candidates.size() == 1 ? candidates[0] : 0);
}

U16 pgn_io::find_move_promotion(Board& b, int pp, int to, int fp, bool isCapture)
{
  std::vector<U16> candidates;
  for (MoveGenerator mvs(b); !mvs.end(); ++mvs)
    {
      U16 m = mvs.move();
      int type = int((m & 0xf000) >> 12);
      if (isCapture) type -= 4;
      int f = get_from(m);
      int p = b.piece_on(f);
      if (get_to(mvs.move()) == to && (fp == p) && (pp == type)) candidates.push_back(m);
    }
  return (candidates.size() == 1 ? candidates[0] : 0);
}

int pgn_io::to_square(std::string& s)
{
  int len = s.size();
  if (len < 2) return -1;

  std::string tostr = s.substr(len - 2, len);
  int to = -1;
  for (int j = 0; j < 64; ++j)
    {
      if (SanSquares[j] == tostr)
	{
	  to = j; break;
	}
    }
  return to;
}

int pgn_io::parse_piece(char& c)
{
  if (c == 'o' || c == '0') return -1; // castle move

  int i = 0; // san piece array starts at 7 for uppercase pieces (non pawn) 
  while (SanPiece[i] && i < 7)
    {
      if (c == SanPiece[i])
	{
	  return i; // piece type between 0-6;
	}
      ++i;
    }
  return 0; // pawn
}

void pgn_io::pgn_strip(std::string& move)
{
  std::string result = "";
  for (unsigned int j = 0; j < move.size(); ++j)
    {
      if (move[j] == '!' || move[j] == '?' ||
	  move[j] == '+' || move[j] == '#' ||
	  move[j] == '[' || move[j] == ']' ||
	  move[j] == '"') continue;
      //result += tolower(move[j]); // not safe to convert to lowercase (bxa4 and Bxa4 for example)
      result += move[j];
    }
  move = result;
}

std::string pgn_io::find(const char * fen)
{
  srand(time(NULL));
  if (!ofile) return "";
  if (!book->compute_key(fen)) return "";
  int stm = book->whos_move;
  U64 k = book->key();
  k = k >> 32;
  size_t offset = 0; size_t sz = ofile->tellg();
  db_entry * e = new db_entry();
  size_t low = 0; size_t high = sz / sizeof(db_entry);
  //printf("struct size = %lu bytes, high = %lu\n", sizeof(db_entry), high);
  printf(" computed key == %lu .. low(%lu), high(%lu), sz(%lu)\n", k, low, high, sz);
  while (low < high)
    {
      offset = low + floor((high - low) / 2);
      ofile->seekg(offset*sizeof(db_entry));
      ofile->read((char*)e, sizeof(db_entry));

      if (k < e->key)
	{
	  //printf("(k < key) = %lu %lu, low(%lu), high(%lu), offset(%lu)\n", k, e->key, low, high, offset);
	  low = offset + 1;
	}
      else if (k > e->key)
	{
	  //printf("(k > key) = %lu %lu, low(%lu), high(%lu), offset(%lu)\n", k, e->key, low, high, offset);
	  high = offset - 1;
	}
      else if (k == e->key)
	{
	  break;
	}
    }

  if (e->key == k) {

    while (e->key == k) {
      offset -= 1;
      ofile->seekg(offset*sizeof(db_entry));
      ofile->read((char*)e, sizeof(db_entry));
    }
    offset += 1;
    ofile->seekg(offset*sizeof(db_entry));
    ofile->read((char*)e, sizeof(db_entry));

    std::vector<std::string> responses;
    std::vector<int * > results;

    while (e->key == k) {
      int type = int((e->move & 0xf000) >> 12);
      std::string response = SanSquares[get_from(e->move)] + SanSquares[get_to(e->move)];
      bool exists = false;
      for (unsigned int j = 0; j < responses.size(); ++j) {
	if (responses[j] == response)
	  {
	    results[j][type]++;
	    exists = true;
	    break;
	  }
      }
      if (!exists) {
	int * r = new int[3];
	r[0] = 0; r[1] = 0; r[2] = 0; r[type]++;
	results.push_back(r);
	responses.push_back(response);
      }

      offset += 1;
      ofile->seekg(offset*sizeof(db_entry));
      ofile->read((char*)e, sizeof(db_entry));
    }

    // sort top 4 moves based on best winning chances for stm
    std::vector<float> win_percentages;
    for (unsigned int j = 0; j < responses.size(); ++j) {
      float tot = (float)(results[j][1] + results[j][0] + results[j][2]);
      if (tot >= 400) win_percentages.push_back(100.0 * (float)(stm == WHITE ? results[j][1] : results[j][2]) / tot);

      printf("..%s  %d %d %d\n", responses[j].c_str(), results[j][1], results[j][0], results[j][2]);

    }
    std::sort(win_percentages.begin(), win_percentages.end(), MoveSort);
    std::vector<int>max_indices;
      
    for (int j = 0; j < MIN((int)win_percentages.size(), 4); ++j) {
      float p = win_percentages[j];
      for (unsigned int i = 0; i < responses.size(); ++i) {
	float rp = 100.0 * (float)(stm == WHITE ? results[i][1] : results[i][2]) / (float)(results[i][1] + results[i][0] + results[i][2]);

	if (p == rp) max_indices.push_back(i);
      }
    }
    int idx = -1; //rand()%responses.size();
    if (max_indices.size() > 0) idx = rand() % MIN((int)win_percentages.size(), 4);
    return (idx < 0 ? "" : responses[idx]);
  }
  else printf("..nothing found\n");
  return "";
}

bool pgn_io::write() {
  if (!ofile || !data) return false;
  printf("..writing to db file\n");
  // sort data on position keys  
  std::sort(data, data + nb_elements, PGNGreaterThan);
  size_t offset = 0; int count = 0;
  for (unsigned int j = 0; j < nb_elements; ++j) {
    db_entry e;
    e.key = data[j].key;
    for (unsigned int m = 0; m < data[j].moves.size(); ++m, ++count) {
      e.move = data[j].moves[m];
      ofile->seekp(offset);
      ofile->write((char*)&e, sizeof(e));
      offset += sizeof(e);
    }
  }
  printf("..finished writing %d moves to db file\n", count);
  ofile->close();
  return true;
}
