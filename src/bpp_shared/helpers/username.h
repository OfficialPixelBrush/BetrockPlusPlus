/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * 
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once

#include <string>

inline bool IsValidUsername(const std::string& _username, bool _allowDotPrefix = false) {
	if (_username.size() < 3 || _username.size() > 16)
		return false;
	size_t index = 0;
	for (unsigned char c : _username) {
		const bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' ||
		                (index == 0 && c == '.' && _allowDotPrefix);
		index++;
		if (!ok)
			return false;
	}
	return true;
}
