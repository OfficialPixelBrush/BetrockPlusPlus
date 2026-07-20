/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#pragma once
#include <string>

// Some helper functions to provide reliable
// conversion from and to UTF-8 and UCS-2,
// as it's needed by Minecraft for various things,
// such as NBT and Packet data

std::string ToUTF8(std::u16string _str);
char32_t DecodeUTF8Char(const std::string& _s, size_t& _i);
std::u16string ToUCS2(std::string _str);