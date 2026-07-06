/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
*/

#pragma once
#include <string>

// Some helper functions to provide reliable
// conversion from and to UTF-8 and UCS-2,
// as it's needed by Minecraft for various things,
// such as NBT and Packet data

std::string ToUTF8(std::u16string str);
char32_t DecodeUTF8Char(const std::string& s, size_t& i);
std::u16string ToUCS2(std::string str);