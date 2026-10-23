/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include <cstdint>
#include <cstring>

#ifdef _MSC_VER
#include <cstdlib>
#define __builtin_bswap16(x) _byteswap_ushort(x)
#define __builtin_bswap32(x) _byteswap_ulong(x)
#define __builtin_bswap64(x) _byteswap_uint64(x)

#include <__msvc_int128.hpp>
using __int128_t = std::_Signed128;
using __uint128_t = std::_Unsigned128;
#endif

// Convert values between host byte order and network/file big-endian order.
// Byte swapping is only necessary on little-endian hosts. Keep the fallback
// runtime check for compilers that do not expose byte-order macros.
inline bool BppHostIsLittleEndian() {
#if defined(_WIN32) || defined(_WIN64)
	return true;
#elif defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
	return __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__;
#elif defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN)
	return __BYTE_ORDER == __LITTLE_ENDIAN;
#else
	const uint16_t value = 1;
	return *reinterpret_cast<const uint8_t*>(&value) == 1;
#endif
}

template <typename T>
inline T BppByteswapIfLittleEndian(T value) {
	if (!BppHostIsLittleEndian())
		return value;

	if constexpr (sizeof(T) == 1) {
		return value;
	} else if constexpr (sizeof(T) == 2) {
		uint16_t tmp;
		std::memcpy(&tmp, &value, sizeof(tmp));
		tmp = __builtin_bswap16(tmp);
		std::memcpy(&value, &tmp, sizeof(tmp));
	} else if constexpr (sizeof(T) == 4) {
		uint32_t tmp;
		std::memcpy(&tmp, &value, sizeof(tmp));
		tmp = __builtin_bswap32(tmp);
		std::memcpy(&value, &tmp, sizeof(tmp));
	} else if constexpr (sizeof(T) == 8) {
		uint64_t tmp;
		std::memcpy(&tmp, &value, sizeof(tmp));
		tmp = __builtin_bswap64(tmp);
		std::memcpy(&value, &tmp, sizeof(tmp));
	}
	return value;
}
