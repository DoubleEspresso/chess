#include "book.h"


Book::Book(char * fen) : b(0), e(0)
{
  
}

~Book::Book()
{
  if (b) { delete b; b = 0; }
  if (e) { delete e; e = 0; }
}

int Book::piece_idx(int type, int color, int r, int c)
{
  type = (color == WHITE ? 2 * type + 1 : 2 * type);
  return 64 * type + 8 * r + c;
}

int Book::piece_idx(int type, int color, int s)
{
  type = (color == WHITE ? 2 * type + 1 : 2 * type);
  int r = ROW(s); int c = COL(s);
  return 64 * type + 8 * r + c;
}

bool Book::load_from_fen(char* fen)
{
  int len = strlen(fen);
  int s = 56; // A8 square
  int offset = 0;
  if (e == 0) e = new polyglot_entry();

  for (int j = 0 ; j < len; ++j, ++offset)
    {
      char c = fen[j];
      if (c == ' ') break; // finished with first segment of the fen string

      if (isdigit(c)) s += int(c - '0');
      else
	{
	  switch (c)
	    {
	    case '/':
	      s -= 16;
	      break;
	    default:
	      set_piece(c, s);
	      s++;	      
	    }
	}
    }
}


void Book::set_piece(char& c, int s)
{
  
}


