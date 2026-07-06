/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
*/

// Some helper functions to provide reliable
// conversion from and to UTF-8 and UCS-2,
// as it's needed by Minecraft for various things,
// such as NBT and Packet data

#include "logger.h"
#include <cstdint>
#include <string>

static std::string ToUTF8(std::u16string str) {
	// UCS-2 limits all values to only be 0x0000 to 0xFFFF
	// in UTF-16 land, 0xDC00–0xDFFF has a special purpose,
	// which we will ignore
	std::string out;
	for (char16_t& c16 : str) {
		if (c16 <= 0x7F) {
			out.push_back(static_cast<char>(c16));
		} else if (c16 <= 0x7FF) {
			out.push_back(static_cast<char>(0xC0 | (c16 >> 6)));
			out.push_back(static_cast<char>(0x80 | (c16 & 0x3F)));
		} else if (c16 <= 0xFFFF) {
			out.push_back(static_cast<char>(0xE0 | (c16 >> 12)));
			out.push_back(static_cast<char>(0x80 | ((c16 >> 6) & 0x3F)));
			out.push_back(static_cast<char>(0x80 | (c16 & 0x3F)));
		}
	}
	return out;
}

static char32_t DecodeUTF8Char(const std::string& s, size_t& i) {
	unsigned char c = s[i];

	if (c < 0x80) {
		return s[i++];
	}

	if ((c & 0xE0) == 0xC0) {
		char32_t cp = ((s[i] & 0x1F) << 6) | (s[i + 1] & 0x3F);
		i += 2;
		return cp;
	}

	if ((c & 0xF0) == 0xE0) {
		char32_t cp = ((s[i] & 0x0F) << 12) | ((s[i + 1] & 0x3F) << 6) | (s[i + 2] & 0x3F);
		i += 3;
		return cp;
	}

	if ((c & 0xF8) == 0xF0) {
		char32_t cp = ((s[i] & 0x07) << 18) | ((s[i + 1] & 0x3F) << 12) | ((s[i + 2] & 0x3F) << 6) | (s[i + 3] & 0x3F);
		i += 4;
		return cp;
	}
}

static std::u16string ToUCS2(std::string str) {
	std::u16string out;

	for (size_t i = 0; i < str.size();) {
		char32_t cp = DecodeUTF8Char(str, i);

		if (cp > 0xFFFF) {
			GlobalLogger().warn << "Code point not representable in UCS-2\n";
			// Spit out whatever we managed to get
			return out;
		}

		// optionally reject surrogate range too
		if (cp >= 0xD800 && cp <= 0xDFFF) {
			GlobalLogger().warn << "Invalid Unicode scalar\n";
			// Spit out whatever we managed to get
			return out;
		}

		out.push_back(static_cast<char16_t>(cp));
	}

	return out;
}