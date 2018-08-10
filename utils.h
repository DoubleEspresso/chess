#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <random>
#include <climits>
#include <memory>

// std::make_unique is part of c++14
template<typename T, typename... Args>
  std::unique_ptr<T> make_unique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

namespace util {  
  inline int row(const int& r) { return (r >> 3); }
  inline int col(const int& c) { return c & 7; }
  inline int row_dist(const int& s1, const int& s2) { return abs(row(s1) - row(s2)); }
  inline int col_dist(const int& s1, const int& s2) { return abs(col(s1) - col(s2)); }
  inline bool on_board(const int& s1) { return s1 >= 0 && s1 <= 63; }
  inline bool same_row(const int& s1, const int& s2) { return row(s1) == row(s2); }
  inline bool same_col(const int& s1, const int& s2) { return col(s1) == col(s2); }
  inline bool on_diagonal(const int& s1, const int& s2) { return col_dist(s1, s2) == row_dist(s1, s2); }
  
  class rand {
    std::mt19937 gen;
    std::uniform_int_distribution<unsigned int> dis;
    public:
    rand() : gen(0), dis(0, UINT_MAX) { }
    
    unsigned int next() { return dis(gen); }
  };     
  
}

#endif
