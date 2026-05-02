/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once
#include <cstdint>

// Just a virtual container
struct ItemStack {
    int16_t id = 0;
    int8_t count = 0;
    int16_t data = 0; // This is "damage" in the og java but data makes more sense for what this is used for

    bool operator==(const ItemStack& o) const {
        return id == o.id && count == o.count && data == o.data;
    }
    bool operator!=(const ItemStack& o) const { return !(*this == o); }
};