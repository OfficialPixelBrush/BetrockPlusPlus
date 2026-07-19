/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/
#include "ucs2.h"
#include "logger.h"

// Turn a UCS-2 String into a UTF-8 String
std::string ToUTF8(std::u16string str) {
	// UCS-2 limits all values to only be 0x0000 to 0xFFFF
	// in UTF-16 land, 0xDC00–0xDFFF have a special purpose,
	// which we will ignore
	std::string out;
	for (size_t i = 0; i < str.size(); i++) {
		const char16_t c = str[i];
		if (c <= 0x7F) {
			out.push_back(static_cast<char>(c));
		} else if (c <= 0x7FF) {
			out.push_back(static_cast<char>(0xC0 | (c >> 6)));
			out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
		} else { // c <= 0xFFFF
			out.push_back(static_cast<char>(0xE0 | (c >> 12)));
			out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
			out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
		}
	}
	return out;
}

// Decode a UTF-8 Character into a singular UCS-2 character
char32_t DecodeUTF8Char(const std::string& s, size_t& i) {
	const unsigned char c = s[i];

	// Try to parse as a one-byte character (e.g. ASCII)
	if (c < 0x80) {
		return s[i++];
	}
	// Try to parse as a two-byte character
	if ((c & 0xE0) == 0xC0) {
		char32_t cp = ((s[i] & 0x1F) << 6) | (s[i + 1] & 0x3F);
		i += 2;
		return cp;
	}
	// Try to parse as a three-byte character
	if ((c & 0xF0) == 0xE0) {
		char32_t cp = ((s[i] & 0x0F) << 12) | ((s[i + 1] & 0x3F) << 6) | (s[i + 2] & 0x3F);
		i += 3;
		return cp;
	}
	// Try to parse as a four-byte character
	if ((c & 0xF8) == 0xF0) {
		char32_t cp = ((s[i] & 0x07) << 18) | ((s[i + 1] & 0x3F) << 12) | ((s[i + 2] & 0x3F) << 6) | (s[i + 3] & 0x3F);
		i += 4;
		return cp;
	}
	// All other parsing failed,
	// return Unknown/Unrecognized Character value
	i++;
	return 0xFFFD;
}

// Turn a UTF-8 String into a UCS-2 String
std::u16string ToUCS2(std::string str) {
	std::u16string out;

	for (size_t i = 0; i < str.size();) {
		char32_t cp = DecodeUTF8Char(str, i);

		if (cp > 0xFFFF) {
			GlobalLogger().m_warn << "Code point not representable in UCS-2\n";
			// Spit out whatever we managed to get
			return out;
		}

		// optionally reject surrogate range too
		if (cp >= 0xD800 && cp <= 0xDFFF) {
			GlobalLogger().m_warn << "Invalid Unicode scalar\n";
			// Spit out whatever we managed to get
			return out;
		}

		out.push_back(static_cast<char16_t>(cp));
	}

	return out;
}