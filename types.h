#pragma once
#ifndef TYPES_H
#define TYPES_H

#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <iostream>

#ifdef _MSC_VER
#include <cstdint>
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;

typedef int16_t int16;
typedef const uint64_t C64;
typedef const uint32_t C32;
typedef uint64_t U64;
typedef uint32_t U32;
typedef uint16_t U16;
typedef uint8_t U8;

#else
#include <stdint.h>
typedef int16_t int16;
typedef const uint64_t C64;
typedef const uint32_t C32;
typedef uint64_t U64;
typedef uint32_t U32;
typedef uint16_t U16;
typedef uint8_t U8;
#endif

// enums
enum Piece { pawn, knight, bishop, rook, queen, king, pieces, no_piece };
enum Color { white, black, colors, no_color };
enum Square {
  A1, B1, C1, D1, E1, F1, G1, H1,
  A2, B2, C2, D2, E2, F2, G2, H2,
  A3, B3, C3, D3, E3, F3, G3, H3,
  A4, B4, C4, D4, E4, F4, G4, H4,
  A5, B5, C5, D5, E5, F5, G5, H5,
  A6, B6, C6, D6, E6, F6, G6, H6,
  A7, B7, C7, D7, E7, F7, G7, H7,
  A8, B8, C8, D8, E8, F8, G8, H8,
  squares, no_square
};
enum Row { r1, r2, r3, r4, r5, r6, r7, r8, rows, no_row };
enum Col { A, B, C, D, E, F, G, H, cols, no_col };

const std::string SanSquares[64] =
  {
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
  };

// enum type enabled iterators
template<typename T> struct is_enum : std::false_type {};
template<> struct is_enum<Piece> : std::true_type {};
template<> struct is_enum<Color> : std::true_type {};
template<> struct is_enum<Square> : std::true_type {};
template<> struct is_enum<Row> : std::true_type {};
template<> struct is_enum<Col> : std::true_type {};

template<typename T, 
  typename = typename std::enable_if<is_enum<T>::value>::type >
  int operator++(T& e) {
  e = T((int)e + 1); return int(e);
}

template<typename T, 
  typename = typename std::enable_if<is_enum<T>::value>::type >
  int operator--(T& e) {
  e = T((int)e - 1); return int(e);
}  


template<typename T1, typename T2,
  typename = typename std::enable_if<is_enum<T1>::value>::type >
  T1& operator-=(T1& lhs, const T2& rhs) {
  lhs = T1(lhs - rhs); return lhs;
}

template<typename T1, typename T2,
  typename = typename std::enable_if<is_enum<T1>::value>::type >
  T1& operator+=(T1& lhs, const T2& rhs) {
  lhs = T1(lhs + rhs); return lhs;
}
  
  
// arrays for iteration
const std::vector<Piece> Pieces { Piece::pawn, Piece::knight, Piece::bishop, Piece::rook, Piece::queen, Piece::king, Piece::pieces, Piece::no_piece };
const std::vector<Color> Colors { Color::white, Color::black, Color::colors, Color::no_color };
const std::vector<char> SanPiece{'P','N','B','R','Q','K','p','n','b','r','q','k'};
const std::vector<char> SanCols{'a','b','c','d','e','f','g','h'};
const std::map<char, U16> CastleRights {{'K', 1}, {'Q', 2}, {'k', 4}, {'q', 8}, {'-', 0}, {'\0', 0}, {' ', 0}};
#endif
