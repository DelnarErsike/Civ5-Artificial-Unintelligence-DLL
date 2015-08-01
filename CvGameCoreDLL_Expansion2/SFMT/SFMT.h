#pragma once
/**
 * @file SFMT.h
 *
 * @brief SIMD oriented Fast Mersenne Twister(SFMT) pseudorandom
 * number generator using C structure.
 *
 * @author Mutsuo Saito (Hiroshima University)
 * @author Makoto Matsumoto (The University of Tokyo)
 *
 * Copyright (C) 2006, 2007 Mutsuo Saito, Makoto Matsumoto and Hiroshima
 * University.
 * Copyright (C) 2012 Mutsuo Saito, Makoto Matsumoto, Hiroshima
 * University and The University of Tokyo.
 * All rights reserved.
 *
 * The 3-clause BSD License is applied to this software, see
 * LICENSE.txt
 *
 * @note We assume that your system has inttypes.h.  If your system
 * doesn't have inttypes.h, you have to typedef uint32_t and uint64_t,
 * and you have to define PRIu64 and PRIx64 in this file as follows:
 * @verbatim
 typedef unsigned int uint32_t
 typedef unsigned long long uint64_t
 #define PRIu64 "llu"
 #define PRIx64 "llx"
@endverbatim
 * uint32_t must be exactly 32-bit unsigned integer type (no more, no
 * less), and uint64_t must be exactly 64-bit unsigned integer type.
 * PRIu64 and PRIx64 are used for printf function to print 64-bit
 * unsigned int and 64-bit unsigned int in hexadecimal format.
 */

// C++'ified for CvGameCoreDLL by Delnar_Ersike

#ifndef SFMTST_H
#define SFMTST_H

#include <stdio.h>
#include <assert.h>

typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;
#define inline __inline

#define PRIu64 "I64u"
#define PRIx64 "I64x"
#define ALIGN16 __declspec(align(16))

#include "SFMT-params.h"

/*------------------------------------------
  128-bit SIMD like data type for standard C
  ------------------------------------------*/
#include <emmintrin.h>

/** 128-bit data structure */
ALIGN16 union W128_T {
	ALIGN16 uint32_t u[4];
	ALIGN16 uint64_t u64[2];
	ALIGN16 __m128i si;
};

/** 128-bit data type */
typedef ALIGN16 union W128_T w128_t;

/**
 * SFMT internal state
 */
ALIGN16 struct SFMT_T {
    /** the 128-bit internal state array */
	ALIGN16 w128_t state[SFMT_N];
    /** index counter to the 32-bit internal state array */
	ALIGN16 int idx;
};

typedef ALIGN16 struct SFMT_T sfmt_t;

class SFMersenneTwister
{
public:
	// Constructors
	SFMersenneTwister()
	{
		sfmt_init_gen_rand(0);
	};
	SFMersenneTwister(uint32_t seed)
	{
		sfmt_init_gen_rand(seed);
	};
	SFMersenneTwister(uint32_t * init_key, int key_length)
	{
		sfmt_init_by_array(init_key, key_length);
	};
	SFMersenneTwister(const SFMersenneTwister& source)
	{
		m_sfmt.idx = source.m_sfmt.idx;
		memcpy(&m_sfmt.state, &source.m_sfmt.state, sizeof(w128_t) * SFMT_N);
	};

	// Destructor
	~SFMersenneTwister(){};

	bool operator==(const SFMersenneTwister& rhs) const;
	bool operator!=(const SFMersenneTwister& rhs) const;
	inline SFMersenneTwister& operator=(const SFMersenneTwister& rhs)
	{
		if (&rhs != this)
		{
			m_sfmt.idx = rhs.m_sfmt.idx;
			memcpy(&m_sfmt.state, &rhs.m_sfmt.state, sizeof(w128_t) * SFMT_N);
		}
		return *this;
	};

	inline static int idxof(int i);
	inline static uint32_t func1(uint32_t x);
	inline static uint32_t func2(uint32_t x);
	inline static const char * sfmt_get_idstring();
	inline static int sfmt_get_min_array_size32();
	inline static int sfmt_get_min_array_size64();

	void sfmt_init_gen_rand(uint32_t seed);
	void sfmt_init_by_array(uint32_t * init_key, int key_length);
	void period_certification();

	void sfmt_fill_array32(uint32_t * array, int size);
	void sfmt_fill_array64(uint64_t * array, int size);

	inline void gen_rand_array(w128_t *array, int size);
	void sfmt_gen_rand_all();

	/**
	 * This function generates and returns 32-bit pseudorandom number.
	 * init_gen_rand or init_by_array must be called before this function.
	 * @param sfmt SFMT internal state
	 * @return 32-bit pseudorandom number
	 */
	inline uint32_t sfmt_genrand_uint32()
	{
		uint32_t * psfmt32 = &m_sfmt.state[0].u[0];

		if (m_sfmt.idx >= SFMT_N32) {
			sfmt_gen_rand_all();
			m_sfmt.idx = 0;
		}
		uint32_t r = psfmt32[m_sfmt.idx++];
		return r;
	};

	/**
	 * This function generates and returns 64-bit pseudorandom number.
	 * init_gen_rand or init_by_array must be called before this function.
	 * The function gen_rand64 should not be called after gen_rand32,
	 * unless an initialization is again executed.
	 * @param sfmt SFMT internal state
	 * @return 64-bit pseudorandom number
	 */
	inline uint64_t sfmt_genrand_uint64()
	{
		uint64_t * psfmt64 = &m_sfmt.state[0].u64[0];
		assert(sfmt->idx % 2 == 0);

		if (m_sfmt.idx >= SFMT_N32) {
			sfmt_gen_rand_all();
			m_sfmt.idx = 0;
		}
		uint64_t r = psfmt64[m_sfmt.idx / 2];
		m_sfmt.idx += 2;
		return r;
	};

	/* =================================================
	   The following real versions are due to Isaku Wada
	   ================================================= */
	/**
	 * converts an unsigned 32-bit number to a double on [0,1]-real-interval.
	 * @param v 32-bit unsigned integer
	 * @return double on [0,1]-real-interval
	 */
	inline static double sfmt_to_real1(uint32_t v)
	{
		return v * (1.0 / 4294967295.0);
		/* divided by 2^32-1 */
	};

	/**
	 * generates a random number on [0,1]-real-interval
	 * @param sfmt SFMT internal state
	 * @return double on [0,1]-real-interval
	 */
	inline double sfmt_genrand_real1()
	{
		return sfmt_to_real1(sfmt_genrand_uint32());
	}

	/**
	 * converts an unsigned 32-bit integer to a double on [0,1)-real-interval.
	 * @param v 32-bit unsigned integer
	 * @return double on [0,1)-real-interval
	 */
	inline static double sfmt_to_real2(uint32_t v)
	{
		return v * (1.0 / 4294967296.0);
		/* divided by 2^32 */
	};

	/**
	 * generates a random number on [0,1)-real-interval
	 * @param sfmt SFMT internal state
	 * @return double on [0,1)-real-interval
	 */
	inline double sfmt_genrand_real2()
	{
		return sfmt_to_real2(sfmt_genrand_uint32());
	};

	/**
	 * converts an unsigned 32-bit integer to a double on (0,1)-real-interval.
	 * @param v 32-bit unsigned integer
	 * @return double on (0,1)-real-interval
	 */
	inline static double sfmt_to_real3(uint32_t v)
	{
		return (((double)v) + 0.5)*(1.0 / 4294967296.0);
		/* divided by 2^32 */
	};

	/**
	 * generates a random number on (0,1)-real-interval
	 * @param sfmt SFMT internal state
	 * @return double on (0,1)-real-interval
	 */
	inline double sfmt_genrand_real3()
	{
		return sfmt_to_real3(sfmt_genrand_uint32());
	};

	/**
	 * converts an unsigned 32-bit integer to double on [0,1)
	 * with 53-bit resolution.
	 * @param v 32-bit unsigned integer
	 * @return double on [0,1)-real-interval with 53-bit resolution.
	 */
	inline static double sfmt_to_res53(uint64_t v)
	{
		return v * (1.0 / 18446744073709551616.0);
	};

	/**
	 * generates a random number on [0,1) with 53-bit resolution
	 * @param sfmt SFMT internal state
	 * @return double on [0,1) with 53-bit resolution
	 */
	inline double sfmt_genrand_res53()
	{
		return sfmt_to_res53(sfmt_genrand_uint64());
	};


	/* =================================================
	   The following function are added by Saito.
	   ================================================= */
	/**
	 * generates a random number on [0,1) with 53-bit resolution from two
	 * 32 bit integers
	 */
	inline static double sfmt_to_res53_mix(uint32_t x, uint32_t y)
	{
		return sfmt_to_res53(x | ((uint64_t)y << 32));
	};

	/**
	 * generates a random number on [0,1) with 53-bit resolution
	 * using two 32bit integers.
	 * @param sfmt SFMT internal state
	 * @return double on [0,1) with 53-bit resolution
	 */
	inline double sfmt_genrand_res53_mix()
	{
		uint32_t x = sfmt_genrand_uint32();
		uint32_t y = sfmt_genrand_uint32();
		return sfmt_to_res53_mix(x, y);
	};
	
	sfmt_t m_sfmt;
};

#endif
