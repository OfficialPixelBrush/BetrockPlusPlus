/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/

#pragma once
#include <array>
#include <cstdint>
#include <string>

#include "blocks.h"
#include "items.h"

static constexpr int16_t ITEM_MINIMUM = 255; // IDs above this are pure items
static constexpr int     SLOT_EMPTY = -1;  // sentinel for "no slot found"

extern std::array<std::string, BLOCK_MAX> blockLabels;
extern std::array<std::string, ITEM_MAX - ITEM_MINIMUM> itemLabels;

std::string IdToLabel(int16_t id);