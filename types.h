#pragma once
#ifndef HEDWIG_TYPES_H
#define HEDWIG_TYPES_H

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


#ifdef DEBUG
#define dbg(x) do { x } while(0)
#else
#define dbg(x) do {} while(0)
#endif

#define U64(x) 1ULL<<x


#ifdef _MSC_VER
#  define FORCE_INLINE  __forceinline
#elif defined(__GNUC__)
#  define FORCE_INLINE  inline __attribute__((always_inline))
#else
#  define FORCE_INLINE  inline
#endif

#define align4   __attribute__((aligned(  4)))
#define align8   __attribute__((aligned(  8)))
#define align16  __attribute__((aligned( 16)))
#define align32  __attribute__((aligned( 32)))
#define align64  __attribute__((aligned( 64)))
#define align128 __attribute__((aligned(128)))

#ifdef __linux
#ifdef BUILD32
const bool _64BIT = false;
#else
const bool _64BIT = true;
#endif
#endif

#endif
