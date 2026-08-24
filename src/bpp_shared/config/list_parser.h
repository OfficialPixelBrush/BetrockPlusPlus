/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/

#include <array>
#include <string>
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
static constexpr std::array<std::string, 2> TARGET_PATHS{ "ops.txt", "whitelist.txt" };
std::vector<std::string> Read(Target _target);
bool Write(std::vector<std::string>& _list, Target _target);
}; // namespace ListParser