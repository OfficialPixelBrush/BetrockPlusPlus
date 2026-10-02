/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "weather_system.h"

void WeatherSystem::Init(int32_t _rainTimer, int32_t _thunderTimer, bool _isRaining, bool _isThundering) {
    rainTimer = _rainTimer;
    thunderTimer = _thunderTimer;
    isRaining = _isRaining;
    isThundering = _isThundering;
}

// TODO: Add a hook here that can be passed in via Init
// so the weather system can inform systems directly if they need to be updated
// This could be useful later down the line for the client,
// so it doesn't need to do a weather check every tick.
void WeatherSystem::Tick(Java::Random& _rand) {
    // Rain
    rainTimer--;
    if (rainTimer <= 0) {
        isRaining = !isRaining;
        if (isRaining)
            rainTimer = 12000 + _rand.NextInt(24000 - 12000 + 1);
        else
            rainTimer = 12000 + _rand.NextInt(180000 - 12000 + 1);
    }
    // Thunder
    thunderTimer--;
    if (thunderTimer <= 0) {
        isThundering = !isThundering;
        if (isThundering)
            thunderTimer = 3600 + _rand.NextInt(15600 - 3600 + 1);
        else
            thunderTimer = 12000 + _rand.NextInt(180000 - 12000 + 1);
    }
}

Weather WeatherSystem::GetActive() const noexcept {
    if (isRaining) {
        if (isThundering) {
            return Weather::Thundering;
        }
        return Weather::Raining;
    }
    return Weather::Clear;
}