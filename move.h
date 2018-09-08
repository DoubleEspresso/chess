#ifndef MOVE_H
#define MOVE_H

//---------------------------------------------
// Move Encoding:
// Store move information in a 16-bit word, 
// using the standard 'from'-'to' representation.
// Broken down as follows,
//
// BITS                                DESCRIPTION
// 0-5                                 from sq (0-63)
// 6-11                                to sq   (0-63)
// 12-16                               move data
//
// last 4 bits
// 0000                                quite move 
// 0001                                capture bit
// 1000                                promotion bit
// 0010                                check bit
// 0100                                castle bit
//
// Quiet promotions
// 1000                                queen
// 1010                                rook
// 1100                                bishop
// 1110                                knight
//
// Capture promotions
// 1001                                queen
// 1011                                rook
// 1101                                bishop
// 1111                                knight
//
// Castle bits
// 0100                                castle k.s.
// 0110                                castle q.s.
//
// check info
// 0010                                check from minor (pawn/bishop/knight)
// 0011                                check from major (queen/rook)
//-------------------------------------------------

class movelist {
  U16 m;
  int v; 
 public:
  movelist() {}
  movelist(const U16& mv, int val = 0) : m(mv), v(val) {}
  movelist(const movelist& o);
  movelist(const movelist&& o);
  movelist& operator=(const movelist& o);
  movelist& operator=(const movelist&& o);
  ~movelist() {}

  bool operator()(const movelist& o) const { return o.m == m; }
  bool operator==(const movelist& o) { return o.m == m; }
};

inline bool mlgreater(const movelist& x, const movelist& y) { return x.v > y.v; }


class movegen {
  
};

#endif
