/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once

#include <string>

inline bool IsValidUsername(const std::string& _username) {
	if (_username.size() < 3 || _username.size() > 16)
		return false;
	for (unsigned char c : _username) {
		const bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
		if (!ok)
			return false;
	}
	return true;
}
