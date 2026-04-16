/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * Based on code by Oracle/OpenJDK (1995-2024)
*/

// A reimplementation of the random function that Java provides
// https://docs.oracle.com/javase/8/docs/api/java/util/Random.html

// For more info, cross-reference with JDK source
// https://github.com/openjdk/jdk8u-dev/blob/master/jdk/src/share/classes/java/util/Random.java

#pragma once

#include <chrono>
#include <cstdint>

/**
 * @brief A faithful reimplementation of the Java pseudorandom number generator
 * 
 */
namespace Java {
	class Random {
	private:
		static constexpr int64_t multiplier = 0x5DEECE66D;
		static constexpr int64_t addend = 0xB;
		static constexpr int64_t mask = (1LL << 48) - 1;

		int64_t seed;

		/**
		* @brief Performs a new iteration of the PRNG
		* 
		* @return Pseudorandom 32-bit integer value
		*/
		int32_t next(int32_t bits) {
			seed = (seed * multiplier + addend) & mask;
			return static_cast<int32_t>(seed >> (48 - bits));
		}

	public:
		/**
		* @brief Construct a new Java Random object
		* 
		* @param initialSeed The initial seed value (defaults to current time)
		*/
		Random(int64_t initialSeed) { setSeed(initialSeed); }

		/**
		* @brief Construct a new Java Random object
		* 
		*/
		Random() {
			// Default seed: current time
			setSeed(static_cast<int64_t>(std::chrono::steady_clock::now().time_since_epoch().count()));
		}

		/**
		* @brief Set the Seed value that'll be used for all subsequently generated values
		* 
		* @param s Seed value
		*/
		void setSeed(int64_t s) { seed = (s ^ multiplier) & mask; }

		/**
		* @brief Returns the next int32_t (32-bit integer)
		* 
		* @return Pseudorandom 32-bit integer value
		*/
		int32_t nextInt() { return next(32); }

		/**
		* @brief Returns the next bound int32_t (32-bit integer)
		* 
		* @return Pseudorandom 32-bit integer value
		*/
		int32_t nextInt(int32_t bound) {
			if (bound <= 0)
				throw std::invalid_argument("bound must be positive");

			if ((bound & -bound) == bound) { // power of two
				return int32_t((bound * int64_t(next(31))) >> 31);
			}

			int32_t bits, val;
			do {
				bits = next(31);
				val = bits % bound;
			} while (bits - val + (bound - 1) < 0);
			return val;
		}

		/**
		* @brief Returns the next long (64-bit integer)
		* 
		* @return Pseudorandom 64-bit long value
		*/
		int64_t nextLong() { return (int64_t(next(32)) << 32) + next(32); }

		/**
		* @brief Returns the next pseudorandom double
		* 
		* @return Pseudorandom double value between 0.0 (inclusive) and 1.0 (exclusive)
		*/
		double nextDouble() { return double((int64_t(next(26)) << 27) + next(27)) / double(1LL << 53); }

		/**
		* @brief Returns the next pseudorandom boolean
		* 
		* @return Pseudorandom boolean
		*/
		bool nextBoolean() { return next(1) != 0; }

		/**
		* @brief Returns the next pseudorandom float
		* 
		* @return Pseudorandom float value between 0.0 (inclusive) and 1.0 (exclusive)
		*/
		float nextFloat() { return float(next(24)) / float(1 << 24); }
	};
}