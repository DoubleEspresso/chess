#include "book.h"
#include <string.h>

Book::Book() : e(0)
{
  
}

Book::~Book()
{
  //  if (b) { delete b; b = 0; }
  if (e) { delete e; e = 0; }
}

int Book::piece_idx(int type, int color, int r, int c)
{
  //type = (color == WHITE ? 2 * type + 1 : 2 * type);
  return 64 * type + 8 * r + c;
}

int Book::piece_idx(int type, int color, int s)
{
  //type = (color == WHITE ? 2 * type + 1 : 2 * type);
  int r = ROW(s); int c = COL(s);
  return 64 * type + 8 * r + c;
}

bool Book::compute_key(char* fen)
{
  U64 piece_key = 0ULL;
  int len = strlen(fen);
  int s = 56; // A8 square
  int offset = 0;
  if (e == 0) e = new polyglot_entry();
  
  // track pawn positions for ep squares
  int bps[8]; int wps[8];
  for(int j=0; j<8; ++j) { bps[j] = -1; wps[j] = -1; }
 

  for (int j = 0 ; j < len; ++j, ++offset)
    {
      char c = fen[j];
      if (c == ' ') break; // finished with first segment of the fen string

      // fen part 1 - the pieces
      if (isdigit(c)) s += int(c - '0');
      else
	{
	  switch (c)
	    {
	    case '/':
	      s -= 16;
	      break;
	    default:
	      update_piece_key(piece_key, c, s, bps, wps);
	      s++;	      
	    }
	}
    }
  if (++offset >= len) return false; // skip ws

  // side to move
  char stm = fen[offset];
  U64 stm_key = 0ULL;
  stm_key ^= (stm == 'w' ? Random64[stm_idx()] : 0ULL );
  offset+=2; // skip wspace
  if (offset >= len) return true;

  // castle rights
  U64 castle_key = 0ULL; 
  while (offset < len && fen[offset] != ' ')
    {
      if (fen[offset] == ' ') break;
      char c = fen[offset]; ++offset;
      switch(c)
	{
	case 'K':
	  castle_key ^= Random64[castle_idx(0)];
	  break;
	case 'Q':
	  castle_key ^= Random64[castle_idx(1)];
	  break;
	case 'k':
	  castle_key ^= Random64[castle_idx(2)];
	  break;
	case 'q':
	  castle_key ^= Random64[castle_idx(3)];
	  break;
	default:
	  break;
	}
    }
  ++offset; // skip ws
  if (offset >= len) return true;

  // ep-square
  char ep_col = fen[offset];
  bool has_ep = false;
  U64 ep_key = 0ULL;
  if (ep_col >= 'a' && ep_col <= 'h') 
    {
      int col = int(ep_col - 'a');
      int row = int(fen[offset+1] - '1');
      int sq = (8*row + col + (stm == 'w' ? -8 : 8)); // sq of 'moved' ep-pawn

      // check neighbor squares of the 'moved' ep-pawn for enemy pawns that can capture it
      // if nothing can capture the ep-pawn, do not encode the ep-data.
      for (int j=0; j<8; ++j)
	{
	  if (col == 0 || col == 7 )
	    {
	      if (stm == 'w' && col == 0 && wps[j] == sq+1) { has_ep = true; break; }
	      else if (stm == 'w' && col == 7 && wps[j] == sq-1) { has_ep = true; break; }
	      else if (stm == 'b' && col == 0 && bps[j] == sq+1) { has_ep = true; break; }
	      else if (stm == 'b' && col == 7 && bps[j] == sq-1) { has_ep = true; break; }
	    }
	  else 
	    {
	      if (stm == 'w' && ( wps[j] == sq+1 || wps[j] == sq-1) ) { has_ep = true; break; }
	      else if (stm == 'b' && (bps[j] == sq+1 || bps[j] == sq-1)) { has_ep = true; break; }
	    }
	}
      
      if (has_ep) ep_key ^= Random64[ep_idx( int(ep_col - 'a') ) ];
    }

  e->key = piece_key ^ castle_key ^ ep_key ^ stm_key;
  printf("..encoded position key for fen(%s) == %lx\n", fen, e->key);
}


void Book::update_piece_key(U64& k, char& c, int s, int (&bps)[8], int (&wps)[8])
{
  // update piece key
  int idx = -1;
  int piece = -1;
  int color = -1;
  for(int j=0; j<SanPiece.length(); ++j)
    {
      if (c == SanPiece[j])
	{
	  idx = j; break;
	}
    }
  if (idx < 6) // white piece
    {
      piece = 2 * idx + 1;
      color = 0;
    }
  else
    {
      piece = 2* (idx - 6); // black piece
      color = 1;
    }

  // eps square tracking .. don't count eps moves unless
  // an enemy pawn is next to the victim pawn
  if (piece == 0 || piece == 1)
    {
      for (int j=0; j<8; ++j)
	{
	  if ( (bps[j] == -1 && color == 1) || (wps[j]==-1 && color == 0))
	    { 
	      if (color == 1) bps[j] = s;
	      else wps[j] = s; 
	      break; 
	    }
	}
    }
  
  // get index for piecetype in the random array
  k ^= Random64[piece_idx(piece, color, s)];
}


