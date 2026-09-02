/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/

#pragma once

#include <array>
#include <string>
#include <string_view>
#include <vector>

/**
 * @brief Generic way to read and write lists of strings
 * 
 */
namespace ListParser {
enum class Target {
	Operator,
	Whitelist
};
static constexpr std::array<std::string_view, 2> TARGET_PATHS{ "ops.txt", "whitelist.txt" };
std::vector<std::string> Read(Target _target);
bool Write(const std::vector<std::string>& _list, Target _target);
}; // namespace ListParser
