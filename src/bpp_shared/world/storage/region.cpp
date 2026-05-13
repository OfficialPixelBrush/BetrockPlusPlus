/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#include "region.h"

Region::Region(Int2 prpos) {
    this->rpos = prpos;
}

int32_t GetOffset(Int2 cpos) {
    return cpos.x + (REGION_WIDTH * cpos.z);
}

bool Region::Serialize() {
    for (auto& cnk : chunks) {
        cnk.first
    }
}

bool Region::Deserialize() {

}