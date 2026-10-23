/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once

#include <bit>
#include <cstdint>
#include <type_traits>

#ifdef _MSC_VER
#include <cstdlib>
#define __builtin_bswap16(x) _byteswap_ushort(x)
#define __builtin_bswap32(x) _byteswap_ulong(x)
#define __builtin_bswap64(x) _byteswap_uint64(x)
#include <__msvc_int128.hpp>
using __int128_t = std::_Signed128;
using __uint128_t = std::_Unsigned128;
#endif

template <typename T>
inline T ByteswapAny(T _value) {
	static_assert(std::is_trivially_copyable_v<T>, "byteswap_any: only trivially copyable types allowed");
	if constexpr (sizeof(T) == 1) {
		return _value;
	} else if constexpr (sizeof(T) == 2) {
		uint16_t tmp;
		std::memcpy(&tmp, &_value, 2);
		tmp = __builtin_bswap16(_value);
		std::memcpy(&_value, &tmp, 2);
	} else if constexpr (sizeof(T) == 4) {
		uint32_t tmp;
		std::memcpy(&tmp, &_value, 4);
		tmp = __builtin_bswap32(tmp);
		std::memcpy(&_value, &tmp, 4);
	} else if constexpr (sizeof(T) == 8) {
		uint64_t tmp;
		std::memcpy(&tmp, &_value, 8);
		tmp = __builtin_bswap64(tmp);
		std::memcpy(&_value, &tmp, 8);
	} else {
		static_assert(sizeof(T) <= 8, "byteswap_any: unsupported type size");
	}
	return _value;
}

template <typename T>
constexpr T ToBigEndian(T _value) {
    if constexpr (std::endian::native == std::endian::big)
        return _value;
    else
        return ByteswapAny(_value);
}

template <typename T>
constexpr T FromBigEndian(T _value) {
    return ToBigEndian(_value);
}
