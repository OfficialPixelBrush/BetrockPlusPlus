/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

// Lighter used by the main world.
#pragma once
#include <cstdint>
#include <queue>
#include <numeric_structs.h>
#include "blocks/block_properties.h"

struct WorldManager;

enum class LightType : uint8_t { Sky, Block };
struct LightUpdate { Int3 pos{ 0, 0, 0 }; LightType type; };
struct UnlightUpdate { Int3 pos{ 0, 0, 0 }; LightType type; int val; };

struct Lighter {
public:
    void propagateLightAt(int x, int y, int z, LightType type, WorldManager& world);
    void unlightAt(int x, int y, int z, LightType type, WorldManager& world);
    void processLightQueue(WorldManager& world);
    void scheduleLightUpdate(Int3 pos, LightType type) {
        if (pos.y < 0 || pos.y >= 128) return;
        lightQueue.push_back({ pos, type });
    }
private:
    std::deque<LightUpdate> lightQueue;
    std::deque<UnlightUpdate> unlightQueue;
};
