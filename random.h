#pragma once
#ifndef HEDWIG_RANDOM_H
#define HEDWIG_RANDOM_H

#include "stdint.h"

// -------------------------------------------
// prng suited for zobrist hashing
// based on the code by Bob Jenkins.
// See :
// http://chessprogramming.wikispaces.com/Bob+Jenkins
// -------------------------------------------
#define rot(x,k) ( ((x)<<(k)) | ( (x) >> (64-(k))) )
struct Rand
{

public:
	Rand(uint64_t seed) { init(seed); };
	Rand() { x = 123456789; y = 362436069; z = 521288629; };
	~Rand() {};

	// initialization
	void init(uint64_t seed) {
		a = 0xf1ea5eed;
		b = c = d = seed;
		for (int j = 0; j<20; j++)
			(void) rand64();
	}

	// return random number
	uint64_t rand64() {
		uint64_t e = a - rot(b, 7);
		a = b ^ rot(c, 13);
		b = c + rot(d, 37);
		c = d + e;
		d = e + a;
		return d;
	};

	// Marsaglia's xorshf generator
	unsigned long rand(void)
	{
		// period 2^96-1
		unsigned long t;
		x ^= x << 16;
		x ^= x >> 5;
		x ^= x << 1;
		t = x;
		x = y;
		y = z;
		z = t ^ x ^ y;
		return z;
	}

private:
	uint64_t a;
	uint64_t b;
	uint64_t c;
	uint64_t d;
	unsigned long x;
	unsigned long y;
	unsigned long z;
};
#undef rot
#endif
