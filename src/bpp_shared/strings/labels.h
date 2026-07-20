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

#include "blocks.h"
#include "items.h"

static constexpr int SLOT_EMPTY = -1; // sentinel for "no slot found"

extern std::array<std::string, BLOCK_MAX> blockLabels;
extern std::array<std::string, Items::MAX - Items::THRESHOLD> itemLabels;

std::string IdToLabel(const int16_t _id);
std::string wIdToLabel(const int16_t _id);