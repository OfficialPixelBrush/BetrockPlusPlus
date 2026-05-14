/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once
#include "strings/labels.h"
#include <cstdint>
#include <ostream>
#include <sstream>
#include <string>

// Just a virtual container
struct ItemStack {
    int16_t id = 0;
    int8_t count = 0;
    int16_t data = 0; // This is "damage" in the og java but data makes more sense for what this is used for

    bool operator==(const ItemStack& o) const {
        return id == o.id && count == o.count && data == o.data;
    }
    bool operator!=(const ItemStack& o) const { return !(*this == o); }

    friend std::wostream& operator<<(std::wostream& wos, const ItemStack& val) {
        wos << "(" 
            << static_cast<int64_t>(val.id) << ":"
            << static_cast<int64_t>(val.data) << " x"
            << static_cast<int64_t>(val.count) << ")";
        return wos;
    }

    friend std::ostream& operator<<(std::ostream& os, const ItemStack& val) {
        os << "(" 
            << static_cast<int64_t>(val.id) << ":"
            << static_cast<int64_t>(val.data) << " x"
            << static_cast<int64_t>(val.count) << ")";
        return os;
    }

    std::wstring wstr() const {
        std::wostringstream woss;
        woss << *this;
        return woss.str();
    }

    std::string str() const {
        std::ostringstream oss;
        oss << *this;
        return oss.str();
    }
};