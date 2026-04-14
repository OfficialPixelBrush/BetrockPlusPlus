/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/

#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <cstdint>
#include <memory>
// #include <pixnbt.h>
#include "base_types.h"
#include "blocks.h"
#include "strings/labels.h"

#define ITEM_INVALID -1

// Item
struct Item {
    ItemId      id = ITEM_INVALID;
    ItemAmount  amount = 0;
    ItemDamage  damage = 0; // Also known as metadata

    friend std::ostream& operator<<(std::ostream& os, const Item& i) {
        os << "(" << IdToLabel(i.id) << ": " << int32_t(i.damage) << " x" << int32_t(i.amount) << ")";
        return os;
    }
    
    std::string str() const {
        std::ostringstream oss;
        oss << *this; // Use the overloaded << operator
        return oss.str();
    }
};

// Block Struct
struct Block {
    BlockType type = BLOCK_AIR;
    int8_t meta = 0;
    int8_t blocklight = 0;
    int8_t skylight = 0;

    friend std::ostream& operator<<(std::ostream& os, const Block& b) {
        os << "(" << int32_t(b.type) << ":" << int32_t(b.meta) << ")";
        return os;
    }
    
    std::string str() const {
        std::ostringstream oss;
        oss << *this; // Use the overloaded << operator
        return oss.str();
    }
};