/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "../enums/weather.h"
#include "../base_types.h"
#include "../helpers/java/java_random.h"

// Based on info from https://minecraft.wiki/w/Weather#Java_Edition_mechanics

struct WeatherSystem {
	int32_t rainTimer = 0;
	int32_t thunderTimer = 0;
    bool isRaining : 1 = false;
    bool isThundering : 1 = false;
    void Init(int32_t _rainTimer = 0, int32_t _thunderTimer = 0, bool _isRaining = false, bool _isThundering = false);
    void Tick(Java::Random& _rand);
    Weather GetActive() const noexcept;
};