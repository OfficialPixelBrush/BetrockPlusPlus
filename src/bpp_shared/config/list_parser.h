/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

/**
 * @brief Generic way to read and write lists of strings
 * 
 */
namespace ListParser {
enum class Target : uint8_t {
	Operator,
	Whitelist,
	BannedPlayers,
	BannedIps
};
static constexpr std::array<std::string_view, 4> TARGET_PATHS{ "ops.txt", "whitelist.txt", "banned-players.txt",
	                                                           "banned-ips.txt" };
std::vector<std::string> Read(Target _target);
bool Write(const std::vector<std::string>& _list, Target _target);
}; // namespace ListParser
