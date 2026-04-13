#pragma once
#include <array>
#include <cstdint>
#include <string>

#include "blocks.h"
#include "items.h"

extern std::array<std::string, BLOCK_MAX> blockLabels;
extern std::array<std::string, ITEM_MAX - ITEM_MINIMUM> itemLabels;

std::string IdToLabel(int16_t id);