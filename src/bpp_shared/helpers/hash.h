/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once

#include <cstddef>
#include <functional>

// Taken from http://www.boost.org/doc/libs/1_55_0/doc/html/hash/reference.html#boost.hash_combine
// TODO: There might be a better alternative worth testing:
// https://stackoverflow.com/questions/35985960/c-why-is-boosthash-combine-the-best-way-to-combine-hash-values
template <typename T>
inline void HashCombine(std::size_t& _h, T _v) {
	_h ^= std::hash<T>{}(_v) + 0x9e3779b97f4a7c15ULL + (_h << 6) + (_h >> 2);
}